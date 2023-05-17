/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPILLMANAGERGMRF_H__
#define __SPILLMANAGERGMRF_H__

#include "Assertions.h"
#include "BuildIR.h"
#include "G4_Opcode.h"

#include <list>
#include <utility>
#include <vector>

// Forward declarations
namespace vISA {
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
} // namespace vISA
struct RegionDesc;
using LiveRangeVec = std::vector<vISA::LiveRange*>;
// Class definitions
namespace vISA {

// New fail safeRA implemenetation
class BoundedRA {
public:
  // "Push" is inserted after prev iter
  // "Pop" is inserted before next iter
  INST_LIST_ITER prev, next;

  static const unsigned int NOT_FOUND = 0xffffffff;

  // Upper threshold of # variables spilled in current iteration
  // to convert current regular (ie, non-fail safe) RA iteration
  // in to fail-safe RA iteration.
  static const unsigned int MaxSpillNumVars = 3;

  // Lower threshold # variables to convert current regular RA
  // iteration in to fail safe RA iteration.
  static const unsigned int LargeProgramSize = 20000;

  static unsigned int getNumPhyVarSlots(G4_Kernel &kernel) {
    return kernel.getNumRegTotal() * kernel.numEltPerGRF<Type_UB>();
  }

  void setInst(const G4_INST *i, G4_BB *bb) {
    curInst = i;
    curBB = bb;
    computeBusy();
  }

  const G4_INST *getInst() const { return curInst; }

  bool isFreeGRF(unsigned int reg) {
    auto &entry = busyGRF[curInst];
    return !entry.test(reg);
  }

  bool isFreeGRFOtherInst(unsigned int reg, const G4_INST *inst) {
    // Used only when looking up free GRFs in non-curInst
    auto &entry = busyGRF[inst];
    return !entry.test(reg);
  }

  void markGRF(unsigned int reg) {
    auto &entry = busyGRF[curInst];
    entry.set(reg, true);
  }

  void markGRFs(unsigned int reg, unsigned int num);

  unsigned int getConsecutiveFree(unsigned int num,
                                  unsigned int forceStart = NOT_FOUND,
                                  bool isIndirect = false) {
    unsigned int start = forceStart, sizeFound = 0;
    unsigned int lastReg = kernel.getNumRegTotal();

    if (start > kernel.getNumRegTotal())
      start = 0;

    bool scannedOnce = false;
    for (unsigned int i = start;; ++i) {
      // No block found in entire GRF file search
      if (scannedOnce && i == forceStart) {
        vISA_ASSERT(false, "no free GRF found in fail safe");
        return NOT_FOUND;
      }

      if (i == lastReg) {
        // Wrap around
        i = 0;
        sizeFound = 0;
        start = i + 1;
        scannedOnce = true;
        continue;
      }

      if (!isFreeGRF(i) || (isIndirect && !isFreeIndir(i))) {
        sizeFound = 0;
        start = i + 1;
        continue;
      }
      ++sizeFound;

      if (sizeFound == num) {
        markGRFs(start, num);
        if (isIndirect) {
          vISA_ASSERT(addrDcl, "expecting non-nullptr addrDcl");
          markIndirBusy(start, num);
        }
        return start;
      }
    }
    return NOT_FOUND;
  }

  unsigned int getFreeGRFIndir(unsigned int num,
                               unsigned int forceStart = NOT_FOUND) {
    return getFreeGRF(num, forceStart, true);
  }

  unsigned int getFreeGRF(unsigned int num, unsigned int forceStart = NOT_FOUND,
                          bool isIndirect = false) {
    // EOT inst requires max GRF - 16 or higher allocation as per HW
    if (!curInst->isEOT())
      return getConsecutiveFree(
          num, forceStart == NOT_FOUND ? reservedGRFStart : forceStart,
          isIndirect);
    else {
      auto freeGRFStart =
          getConsecutiveFree(num, kernel.getNumRegTotal() - 16, isIndirect);
      vISA_ASSERT(freeGRFStart >= (kernel.getNumRegTotal() - 16) &&
                       (freeGRFStart + num) < kernel.getNumRegTotal(),
                   "unexpected EOT allocation");
      return freeGRFStart;
    }
  }

  void setSpillOff(unsigned int off) {
    // This is scratch offset after considering private storage used by IGC
    spillOffset = off;
  }

  void setReservedStart(unsigned int s) { reservedGRFStart = s; }

  void insertPushPop(bool useLSCMsg);

  void markIndirIntfs();
  void setAddrDcl(G4_Declare *a) { addrDcl = a; }
  void resetAddrDcl() { addrDcl = nullptr; }

  void computeAllBusy() {
    for (auto bb : kernel.fg.getBBList())
      for (auto inst : bb->getInstList())
        setInst(inst, bb);
  }

  void markClobbered(unsigned int reg, unsigned int numRegs) {
    // Mark registers as being clobbered in current inst
    auto &entry = clobberedGRFs[curInst];
    for (unsigned int busyReg = reg; busyReg != (reg + numRegs); ++busyReg) {
      entry.insert(busyReg);
    }
  }

  BoundedRA(GlobalRA &ra, const LiveRangeVec &l);

private:
  GlobalRA &gra;
  G4_Kernel &kernel;
  const LiveRangeVec &lrs;
  static const unsigned int bitsetSz = 256;
  // Map each inst -> list of busy GRFs
  std::unordered_map<const G4_INST *, std::bitset<bitsetSz>> busyGRF;
  std::unordered_map<const G4_INST *, std::set<unsigned short>> clobberedGRFs;
  std::unordered_map<G4_Declare *, std::list<const G4_INST *>> busyIndir;

  const G4_INST *curInst = nullptr;
  G4_BB *curBB = nullptr;
  G4_Declare *addrDcl = nullptr;
  unsigned int lastGRF = 1;
  unsigned int spillOffset = NOT_FOUND;
  unsigned int reservedGRFStart = NOT_FOUND;

  void markBusyGRFs();
  void computeBusy() {
    if (!curInst)
      return;
    markBusyGRFs();
  }

  // Mark forbidden registers that are universal (eg, r0)
  void markUniversalForbidden();
  // Forbidden is marked per LR in GRA. In this case, we
  // club forbidden for all operands in a instruction and
  // apply it to the instruction for simplicity.
  void markForbidden(LiveRange *lr);

  void markIndirBusy(unsigned int start, unsigned int num) {
    vISA_ASSERT(busyIndir.find(addrDcl) != busyIndir.end(),
                 "no inst found referencing addrDcl");

    auto &refs = busyIndir[addrDcl];

    // Mark GRFs as busy in all instruction where current
    // address register is used as indirect.
    for (auto inst : refs) {
      for (unsigned int reg = start; reg != (start + num); ++reg)
        busyGRF[inst].set(reg);
    }
  }

  bool isFreeIndir(unsigned int r);
};

class SpillManagerGRF {
public:
  using DECLARE_LIST = std::list<G4_Declare *>;
  using LR_LIST = std::list<LiveRange *>;
  using LSLR_LIST = std::list<LSLiveRange *>;
  using INST_LIST = std::list<G4_INST *, INST_LIST_NODE_ALLOCATOR>;
  typedef struct Edge {
    unsigned first;
    unsigned second;
  } EDGE;
  using INTF_LIST = std::list<EDGE>;

  // Methods

  SpillManagerGRF(GlobalRA &g, unsigned spillAreaOffset, unsigned varIdCount,
                  const LivenessAnalysis *lvInfo, const LiveRangeVec& lrInfo,
                  const Interference *intf, const LR_LIST *spilledLRs,
                  unsigned iterationNo, bool useSpillReg, unsigned spillRegSize,
                  unsigned indrSpillRegSize, bool enableSpillSpaceCompression,
                  bool useScratchMsg, bool avoidDstSrcOverlap);

  SpillManagerGRF(GlobalRA &g, unsigned spillAreaOffset, unsigned varIdCount,
                  const LivenessAnalysis *lvInfo, LSLR_LIST *spilledLSLRs,
                  bool enableSpillSpaceCompression, bool useScratchMsg,
                  bool avoidDstSrcOverlap,
                  const LiveRangeVec &emptyLRs);

  ~SpillManagerGRF() {}

  bool insertSpillFillCode(G4_Kernel *kernel,
                           PointsToAnalysis &pointsToAnalysis);

  void expireRanges(unsigned int idx, std::list<LSLiveRange *> *liveList);

  void updateActiveList(LSLiveRange *lr, std::list<LSLiveRange *> *liveList);

  bool spillLiveRanges(G4_Kernel *kernel);

  unsigned getNumGRFSpill() const { return numGRFSpill; }
  unsigned getNumGRFFill() const { return numGRFFill; }
  unsigned getNumGRFMove() const { return numGRFMove; }
  // return the next cumulative logical offset. This does not non-spilled stuff
  // like private variables placed by IGC (marked by spill_mem_offset) this
  // should only be called after insertSpillFillCode()
  uint32_t getNextOffset() const { return nextSpillOffset_; }
  // return the cumulative scratch space offset for the next spilled variable.
  // This adjusts for scratch space reserved for file scope vars and IGC/GT-pin
  uint32_t getNextScratchOffset() const {
    int offset = nextSpillOffset_;
    getSpillOffset(offset);
    return offset;
  }

  // convert zero-based logical offset into the scratch space offset.
  void getSpillOffset(int &logicalOffset) const {
    logicalOffset += globalScratchOffset;
  }

  static std::tuple<uint32_t, G4_ExecSize>
  createSpillSendMsgDescOWord(const IR_Builder &builder, unsigned int height);

private:
  G4_Declare *getOrCreateAddrSpillFillDcl(G4_RegVar *addrDcl,
                                          G4_Declare *spilledAddrTakenDcl,
                                          G4_Kernel *kernel);
  bool handleAddrTakenSpills(G4_Kernel *kernel,
                             PointsToAnalysis &pointsToAnalysis);
  unsigned int handleAddrTakenLSSpills(G4_Kernel *kernel,
                                       PointsToAnalysis &pointsToAnalysis);
  void insertAddrTakenSpillFill(G4_Kernel *kernel,
                                PointsToAnalysis &pointsToAnalysis);
  void insertAddrTakenLSSpillFill(G4_Kernel *kernel,
                                  PointsToAnalysis &pointsToAnalysis);
  void insertAddrTakenSpillAndFillCode(G4_Kernel *kernel, G4_BB *bb,
                                       INST_LIST::iterator inst_it,
                                       G4_Operand *opnd,
                                       PointsToAnalysis &pointsToAnalysis,
                                       bool spill, unsigned int bbid);
  void insertAddrTakenLSSpillAndFillCode(G4_Kernel *kernel, G4_BB *bb,
                                         INST_LIST::iterator inst_it,
                                         G4_Operand *opnd,
                                         PointsToAnalysis &pointsToAnalysis,
                                         bool spill, unsigned int bbid);
  void prunePointsTo(G4_Kernel *kernel, PointsToAnalysis &pointsToAnalysis);

  void prunePointsToLS(G4_Kernel *kernel, PointsToAnalysis &pointsToAnalysis);

  bool isComprInst(G4_INST *inst) const;

  bool isMultiRegComprSource(G4_SrcRegRegion *src, G4_INST *inst) const;

  unsigned getSendMaxResponseLength() const;

  unsigned getSendMaxMessageLength() const;

  static unsigned getSendDescDataSizeBitOffset();

  unsigned getSendReadTypeBitOffset() const;

  static unsigned getSendWriteTypeBitOffset();

  unsigned getSendScReadType() const;

  unsigned getSendScWriteType() const;

  unsigned getSendOwordReadType() const;
  static unsigned getSendOwordWriteType();
  unsigned getSendExDesc(bool isWrite, bool isScatter) const;

  unsigned getSpillIndex(G4_RegVar *spilledRegVar);

  unsigned getFillIndex(G4_RegVar *spilledRegVar);

  unsigned getTmpIndex(G4_RegVar *spilledRegVar);

  unsigned getMsgSpillIndex(G4_RegVar *spilledRegVar);

  unsigned getMsgFillIndex(G4_RegVar *spilledRegVar);

  unsigned getAddrSpillFillIndex(G4_RegVar *spilledRegVar);

  template <class REGION_TYPE> G4_RegVar *getRegVar(REGION_TYPE *region) const;

  G4_RegFileKind getRFType(G4_RegVar *regvar) const;

  template <class REGION_TYPE>
  G4_RegFileKind getRFType(REGION_TYPE *region) const;

  template <class REGION_TYPE>
  unsigned getRegionOriginOffset(REGION_TYPE *region) const;

  unsigned grfMask() const;

  unsigned hwordMask() const;

  unsigned owordMask() const;

  unsigned dwordMask() const;

  bool owordAligned(unsigned offset) const;

  bool dwordAligned(unsigned offset) const;

  static unsigned cdiv(unsigned dvd, unsigned dvr);

  G4_RegVar *getRegVar(unsigned id) const;

  bool shouldSpillRegister(G4_RegVar *regVar) const;

  unsigned getByteSize(G4_RegVar *regVar) const;

  template <class REGION_TYPE>
  unsigned getSegmentDisp(REGION_TYPE *region, G4_ExecSize execSize);

  unsigned getDisp(G4_RegVar *lRange);

  template <class REGION_TYPE> unsigned getRegionDisp(REGION_TYPE *region);

  unsigned calculateSpillDisp(G4_RegVar *lRange) const;

  unsigned calculateSpillDispForLS(G4_RegVar *regVar) const;

  template <class REGION_TYPE>
  unsigned getMsgType(REGION_TYPE *region, G4_ExecSize execSize);

  template <class REGION_TYPE>
  bool isUnalignedRegion(REGION_TYPE *region, G4_ExecSize execSize);

  template <class REGION_TYPE>
  void calculateEncAlignedSegment(REGION_TYPE *region, G4_ExecSize execSize,
                                  unsigned &start, unsigned &end,
                                  unsigned &type);

  template <class REGION_TYPE>
  unsigned getEncAlignedSegmentByteSize(REGION_TYPE *region,
                                        G4_ExecSize execSize);

  template <class REGION_TYPE>
  unsigned getEncAlignedSegmentDisp(REGION_TYPE *region, G4_ExecSize execSize);

  template <class REGION_TYPE>
  unsigned getEncAlignedSegmentMsgType(REGION_TYPE *region,
                                       G4_ExecSize execSize);

  template <class REGION_TYPE>
  unsigned getSegmentByteSize(REGION_TYPE *region, G4_ExecSize execSize);

  unsigned getRegionByteSize(G4_DstRegRegion *region,
                             G4_ExecSize execSize) const;

  unsigned getRegionByteSize(G4_SrcRegRegion *region,
                             G4_ExecSize execSize) const;

  bool isScalarReplication(G4_SrcRegRegion *region) const;

  bool repeatSIMD16or32Source(G4_SrcRegRegion *region) const;

  G4_Declare *createRangeDeclare(const char *name, G4_RegFileKind regFile,
                                 unsigned short nElems, unsigned short nRows,
                                 G4_Type type, DeclareType kind,
                                 G4_RegVar *base, G4_Operand *repRegion,
                                 G4_ExecSize execSize);

  template <class REGION_TYPE>
  G4_Declare *createTransientGRFRangeDeclare(REGION_TYPE *region, bool isFill,
                                             unsigned index,
                                             G4_ExecSize execSize,
                                             G4_INST *inst);

  G4_Declare *createPostDstSpillRangeDeclare(G4_INST *sendOut);

  G4_Declare *createSpillRangeDeclare(G4_DstRegRegion *spillRegion,
                                      G4_ExecSize execSize, G4_INST *inst);

  G4_Declare *createGRFFillRangeDeclare(G4_SrcRegRegion *fillRegion,
                                        G4_ExecSize execSize, G4_INST *inst);

  G4_Declare *createSendFillRangeDeclare(G4_SrcRegRegion *filledRegion,
                                         G4_INST *sendInst);

  G4_Declare *createTemporaryRangeDeclare(G4_DstRegRegion *fillRegion,
                                          G4_ExecSize execSize,
                                          bool forceSegmentAlignment = false);

  G4_DstRegRegion *createSpillRangeDstRegion(G4_RegVar *spillRangeRegVar,
                                             G4_DstRegRegion *spilledRegion,
                                             G4_ExecSize execSize,
                                             unsigned regOff = 0);

  G4_SrcRegRegion *createFillRangeSrcRegion(G4_RegVar *fillRangeRegVar,
                                            G4_SrcRegRegion *filledRegion,
                                            G4_ExecSize execSize);

  G4_SrcRegRegion *createTemporaryRangeSrcRegion(G4_RegVar *tmpRangeRegVar,
                                                 G4_DstRegRegion *spilledRegion,
                                                 G4_ExecSize execSize,
                                                 unsigned regOff = 0);

  G4_SrcRegRegion *createBlockSpillRangeSrcRegion(G4_RegVar *spillRangeRegVar,
                                                  unsigned regOff = 0,
                                                  unsigned subregOff = 0);

  G4_Declare *createMRangeDeclare(G4_RegVar *regVar);

  G4_Declare *createMRangeDeclare(G4_DstRegRegion *region,
                                  G4_ExecSize execSize);

  G4_Declare *createMRangeDeclare(G4_SrcRegRegion *region,
                                  G4_ExecSize execSize);

  G4_DstRegRegion *createMPayloadBlockWriteDstRegion(G4_RegVar *grfRange,
                                                     unsigned regOff = 0,
                                                     unsigned subregOff = 0);

  G4_DstRegRegion *createMHeaderInputDstRegion(G4_RegVar *grfRange,
                                               unsigned subregOff = 0);

  G4_DstRegRegion *createMHeaderBlockOffsetDstRegion(G4_RegVar *grfRange);

  G4_SrcRegRegion *createInputPayloadSrcRegion();

  G4_Declare *initMHeader(G4_Declare *mRangeDcl);

  G4_Declare *createAndInitMHeader(G4_RegVar *regVar);

  template <class REGION_TYPE>
  G4_Declare *initMHeader(G4_Declare *mRangeDcl, REGION_TYPE *region,
                          G4_ExecSize execSize);

  template <class REGION_TYPE>
  G4_Declare *createAndInitMHeader(REGION_TYPE *region, G4_ExecSize execSize);

  void sendInSpilledRegVarPortions(G4_Declare *fillRangeDcl,
                                   G4_Declare *mRangeDcl, unsigned regOff,
                                   unsigned height, unsigned srcRegOff = 0);

  void sendOutSpilledRegVarPortions(G4_Declare *spillRangeDcl,
                                    G4_Declare *mRangeDcl, unsigned regOff,
                                    unsigned height, unsigned srcRegOff = 0);

  void initMWritePayload(G4_Declare *spillRangeDcl, G4_Declare *mRangeDcl,
                         unsigned regOff, unsigned height);

  void initMWritePayload(G4_Declare *spillRangeDcl, G4_Declare *mRangeDcl,
                         G4_DstRegRegion *spilledRangeRegion,
                         G4_ExecSize execSize, unsigned regOff = 0);

  static unsigned blockSendBlockSizeCode(unsigned regOff);

  unsigned scatterSendBlockSizeCode(unsigned regOff) const;

  G4_Imm *createSpillSendMsgDesc(unsigned regOff, unsigned height,
                                 G4_ExecSize &execSize, G4_RegVar *base = NULL);

  std::tuple<G4_Imm *, G4_ExecSize>
  createSpillSendMsgDesc(G4_DstRegRegion *spilledRangeRegion,
                         G4_ExecSize execSize);

  G4_INST *createAddFPInst(G4_ExecSize execSize, G4_DstRegRegion *dst,
                           G4_Operand *src);

  G4_INST *createMovInst(G4_ExecSize execSize, G4_DstRegRegion *dst,
                         G4_Operand *src, G4_Predicate *predicate = NULL,
                         G4_InstOpts options = InstOpt_WriteEnable);

  G4_INST *createSendInst(G4_ExecSize execSize, G4_DstRegRegion *postDst,
                          G4_SrcRegRegion *payload, G4_Imm *desc, SFID funcID,
                          bool isWrite, G4_InstOpts option);

  bool usesOWOrLSC(G4_DstRegRegion *spilledRegion);

  bool shouldPreloadSpillRange(G4_INST *instContext, G4_BB *parentBB);

  void preloadSpillRange(G4_Declare *spillRangeDcl, G4_Declare *mRangeDcl,
                         G4_DstRegRegion *spilledRangeRegion,
                         G4_ExecSize execSize);

  G4_INST *createSpillSendInstr(G4_Declare *spillRangeDcl,
                                G4_Declare *mRangeDcl, unsigned regOff,
                                unsigned height, unsigned spillOff);

  G4_INST *createSpillSendInstr(G4_Declare *spillRangeDcl,
                                G4_Declare *mRangeDcl,
                                G4_DstRegRegion *spilledRangeRegion,
                                G4_ExecSize execSize, unsigned option);

  G4_Imm *createFillSendMsgDesc(unsigned regOff, unsigned height,
                                G4_ExecSize &execSize, G4_RegVar *base = NULL);

  template <class REGION_TYPE>
  G4_Imm *createFillSendMsgDesc(REGION_TYPE *filledRangeRegion,
                                G4_ExecSize execSize);

  G4_INST *createFillSendInstr(G4_Declare *fillRangeDcl, G4_Declare *mRangeDcl,
                               unsigned regOff, unsigned height,
                               unsigned spillOff);

  G4_INST *createFillSendInstr(G4_Declare *fillRangeDcl, G4_Declare *mRangeDcl,
                               G4_SrcRegRegion *filledRangeRegion,
                               G4_ExecSize execSize);

  G4_SrcRegRegion *getLSCSpillFillHeader(G4_Declare *mRangeDcl,
                                         const G4_Declare *fp, int offset);

  G4_INST *createLSCSpill(G4_Declare *spillRangeDcl, G4_Declare *mRangeDcl,
                          unsigned regOff, unsigned height, unsigned spillOff);

  G4_INST *createLSCSpill(G4_Declare *spillRangeDcl, G4_Declare *mRangeDcl,
                          G4_DstRegRegion *spilledRangeRegion,
                          G4_ExecSize execSize, unsigned option,
                          bool isScatter = false);

  G4_INST *createLSCFill(G4_Declare *fillRangeDcl, G4_Declare *mRangeDcl,
                         unsigned regOff, unsigned height, unsigned spillOff);

  G4_INST *createLSCFill(G4_Declare *fillRangeDcl, G4_Declare *mRangeDcl,
                         G4_SrcRegRegion *filledRangeRegion,
                         G4_ExecSize execSize);

  void replaceSpilledRange(G4_Declare *spillRangeDcl,
                           G4_DstRegRegion *spilledRegion, G4_INST *spilledInst,
                           uint32_t subregOff);

  void replaceFilledRange(G4_Declare *fillRangeDcl,
                          G4_SrcRegRegion *filledRegion, G4_INST *filledInst);

  void insertSpillRangeCode(INST_LIST::iterator spilledInstIter, G4_BB *bb);

  INST_LIST::iterator
  insertSendFillRangeCode(G4_SrcRegRegion *filledRegion,
                          INST_LIST::iterator filledInstIter, G4_BB *bb);

  void insertFillGRFRangeCode(G4_SrcRegRegion *filledRegion,
                              INST_LIST::iterator filledInstIter, G4_BB *bb);

  bool useSplitSend() const;

  int getHWordEncoding(int numHWord) {
    switch (numHWord) {
    case 1:
      return 0;
    case 2:
      return 1;
    case 4:
      return 2;
    case 8:
      return 3;
    default:
      vISA_ASSERT_UNREACHABLE("only 1/2/4/8 HWords are supported");
      return 0;
    }
  }

  ChannelMask getChMaskForSpill(int numHWord) const {
    switch (numHWord) {
    case 1:
    case 2:
      return ChannelMask::createFromAPI(CHANNEL_MASK_R);
    case 4:
      return ChannelMask::createFromAPI(CHANNEL_MASK_RG);
    case 8:
      return ChannelMask::createFromAPI(CHANNEL_MASK_RGBA);
    default:
      vISA_ASSERT_UNREACHABLE("illegal spill size");
      return ChannelMask::createFromAPI(CHANNEL_MASK_R);
    }
  }

  void getOverlappingIntervals(G4_Declare *dcl,
                               std::vector<G4_Declare *> &intervals) const;

  // Data
  GlobalRA &gra;
  IR_Builder *builder_;
  unsigned varIdCount_;
  unsigned latestImplicitVarIdCount_;
  const LivenessAnalysis *lvInfo_;
  const LiveRangeVec &lrInfo_;
  const LR_LIST *spilledLRs_;
  LSLR_LIST *spilledLSLRs_;
  std::vector<unsigned> spillRangeCount_;
  std::vector<unsigned> fillRangeCount_;
  std::vector<unsigned> tmpRangeCount_;
  std::vector<unsigned> msgSpillRangeCount_;
  std::vector<unsigned> msgFillRangeCount_;
  std::vector<unsigned> addrSpillFillRangeCount_;
  unsigned nextSpillOffset_;
  unsigned iterationNo_;
  unsigned bbId_ = UINT_MAX;
  unsigned spillAreaOffset_;
  bool doSpillSpaceCompression;

  bool failSafeSpill_;
  unsigned spillRegStart_;
  unsigned indrSpillRegStart_;
  unsigned spillRegOffset_;
  LSLR_LIST activeLR_;
  std::unordered_set<G4_DstRegRegion *> noRMWNeeded;

  const Interference *spillIntf_;

  // The number of GRF spill.
  unsigned numGRFSpill = 0;

  // The number of GRF fill.
  unsigned numGRFFill = 0;

  // The number of mov.
  unsigned numGRFMove = 0;

  // CISA instruction id of current instruction
  G4_INST *curInst;

  int globalScratchOffset;

  bool useScratchMsg_;
  bool avoidDstSrcOverlap_;
  // spilled declares that represent a scalar immediate (created due to encoding
  // restrictions) We rematerialize the immediate value instead of spill/fill
  // them
  std::unordered_map<G4_Declare *, G4_Imm *> scalarImmSpill;
  // distance to reuse filled scalar imm
  const unsigned int scalarImmReuseDistance = 10;
  // use cache to reuse filled scalar immediates for nearby uses
  // map spilled declare -> fill bb, fill inst, lex id for distance heuristic
  std::unordered_map<G4_Declare*, std::tuple<G4_BB*, G4_INST*, unsigned int>> scalarImmFillCache;

  VarReferences refs;

  // sorted list of all spilling intervals
  std::vector<G4_Declare *> spillingIntervals;

  // analysis pass to assist in spill/fill code gen
  // currently it identifies scalar imm variables that should be re-mat
  // later on we can add detection to avoid unncessary read-modify-write for
  // spills
  void immMovSpillAnalysis();

  // Return true if spillDcl is a re-spill of a rematerialized imm
  bool isScalarImmRespill(G4_Declare *spillDcl) const;
  bool immFill(G4_SrcRegRegion *filledRegion,
               INST_LIST::iterator filledInstIter, G4_BB *bb,
               G4_Declare *spillDcl);

  bool checkUniqueDefAligned(G4_DstRegRegion *dst, G4_BB *defBB);
  bool checkDefUseDomRel(G4_DstRegRegion *dst, G4_BB *bb);
  void updateRMWNeeded();

  bool useLSCMsg = false;
  bool useLscNonstackCall = false;
  bool useLscForScatterSpill = false;

  // Used for new fail safe RA mechanism.
  BoundedRA context;

  // Used if an address-taken variable is spilled.
  // Maps the old AddrExp operand on the spilled variable to the new AddrExp on
  // the temp variable. The old AddrExp will be replaced by the new one when
  // this round of spilling is done.
  std::unordered_map<G4_AddrExp *, G4_AddrExp *> addrTakenSpillFill;

  void setAddrTakenSpillFill(G4_AddrExp *origOp, G4_AddrExp *newOp) {
    addrTakenSpillFill[origOp] = newOp;
  }
  G4_AddrExp *getAddrTakenSpillFill(G4_AddrExp *addrOp) {
    auto iter = addrTakenSpillFill.find(addrOp);
    return iter == addrTakenSpillFill.end() ? nullptr : iter->second;
  }

  bool headerNeeded() const {
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

  void initializeRangeCount(unsigned size);

  // return true if offset for spill/fill message needs to be GRF-aligned
  bool needGRFAlignedOffset() const { return useScratchMsg_ || useSplitSend(); }
}; // class SpillManagerGRF

// Check if the destination region is discontiguous or not.
// A destination region is discontiguous if there are portions of the
// region that are not written and unaffected.
static inline bool isDisContRegion(G4_DstRegRegion *region, unsigned execSize) {
  // If the horizontal stride is greater than 1, then it has gaps.
  // NOTE: Horizontal stride of 0 is not allowed for destination regions.
  return region->getHorzStride() != 1;
}

// Check if the source region is discontiguous or not.
// A source region is discontiguous in there are portions of the region
// that are not read.
static inline bool isDisContRegion(G4_SrcRegRegion *region, unsigned execSize) {
  return region->getRegion()->isContiguous(execSize);
}

// Check if the region is partial or not, i.e does it read/write the
// whole segment.
template <class REGION_TYPE>
static inline bool isPartialRegion(REGION_TYPE *region, unsigned execSize) {
  // If the region is discontiguous then it is partial.
  if (isDisContRegion(region, execSize)) {
    return true;
  } else {
    return false;
  }
}

// Return true if inst is a simple mov with exec size == 1 and imm as src0.
// Def of such instructions are trivially fillable.
static bool immFillCandidate(G4_INST *inst) {
  return inst->opcode() == G4_mov && inst->getExecSize() == g4::SIMD1 &&
         inst->getSrc(0)->isImm() && inst->isWriteEnableInst() &&
         inst->getDst()->getType() == inst->getSrc(0)->getType() &&
         !inst->getPredicate() && !inst->getCondMod() && !inst->getSaturate();
}

G4_SrcRegRegion *getSpillFillHeader(IR_Builder &builder, G4_Declare *decl);

// Class used to analyze spill/fill decisions
class SpillAnalysis {
public:
  SpillAnalysis() = default;
  ~SpillAnalysis();

  void Dump(std::ostream &OS = std::cerr);
  void DumpHistogram(std::ostream &OS = std::cerr);

  unsigned int GetDistance(G4_Declare *Dcl);
  void LoadAugIntervals(DECLARE_LIST &, GlobalRA &);
  void LoadDegree(G4_Declare *Dcl, unsigned int degree);

  void SetLivenessAnalysis(LivenessAnalysis *L) { LA = L; }
  void SetGraphColor(GraphColor *C) { GC = C; }
  void SetSpillManager(SpillManagerGRF *S) { SM = S; }

  void Clear();

  void Do(LivenessAnalysis *L, GraphColor *C, SpillManagerGRF *S);

private:
  VarReferences *Refs = nullptr;

  LivenessAnalysis *LA = nullptr;
  GraphColor *GC = nullptr;
  SpillManagerGRF *SM = nullptr;

  std::unordered_map<G4_Declare *, std::pair<G4_INST *, G4_INST *>>
      AugIntervals;
  std::unordered_map<G4_Declare *, unsigned int> DclDegree;

  std::vector<G4_BB *> GetLiveBBs(G4_Declare *,
                                  std::unordered_map<G4_INST *, G4_BB *> &);
  std::vector<G4_BB *>
  GetIntervalBBs(G4_INST *Start, G4_INST *End,
                 std::unordered_map<G4_INST *, G4_BB *> &InstBBMap);
};

bool isEOTSpillWithFailSafeRA(const IR_Builder &builder, const LiveRange *lr,
                              bool isFailSafeIter);

} // namespace vISA

#endif // __SPILLMANAGERGMRF_H__
