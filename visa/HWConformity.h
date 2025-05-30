/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _HWCONFORMITY_H_
#define _HWCONFORMITY_H_

#include "BuildIR.h"
#include "Common_ISA_util.h"
#include "FlowGraph.h"
#include "G4_IR.hpp"

#include <map>

namespace vISA {
class DPASSrc2RSCache {
public:
  std::vector<G4_Declare *> GRFCache;
  unsigned latestID;
  bool firstDpas;

  DPASSrc2RSCache() {
    latestID = 0;
    firstDpas = true;
    GRFCache.resize(16, nullptr);
  }
  ~DPASSrc2RSCache() {}
};

class PointsToAnalysis;
class HWConformity {
  IR_Builder &builder;
  G4_Kernel &kernel;

  // this must be set before calling the individual fix functions
  G4_BB *curBB = nullptr;

  // This is added for data layout optimization.
  // Currently it only targets packed-byte pattern.
  // Can be extended later for other patterns.
  enum AccessPattern {
    ACCESS_PATTERN_UNDEF = 0,
    ACCESS_PATTERN_PACKED_BYTE = 1,
    ACCESS_PATTERN_INVALID = 2
  };
  std::map<G4_Declare *, AccessPattern> dclAccessPattern;

  AccessPattern getAccessPattern(G4_Declare *dcl) {
    auto iter = dclAccessPattern.find(dcl);
    if (iter == dclAccessPattern.end()) {
      return ACCESS_PATTERN_UNDEF;
    }
    return (*iter).second;
  }
  void setAccessPattern(G4_Declare *dcl, AccessPattern ap) {
    dclAccessPattern[dcl] = ap;
  }
  bool markPackedByteReference(G4_Kernel &kernel, G4_Operand *opnd,
                               G4_INST *inst);
  G4_Operand *fixPackedByteReference(IR_Builder &builder, G4_Operand *opnd);

  // helper functions
  bool hasBadRegion(G4_INST *inst);
  bool canSplitInst(G4_INST *inst, G4_INST *use_op);
  bool splitInstListForByteDst(INST_LIST_ITER it, G4_BB *bb,
                               uint16_t extypesize);

  G4_SrcRegRegion *insertCopyBefore(INST_LIST_ITER it, uint32_t srcNum,
                                    G4_SubReg_Align tmpAlign, G4_BB *bb);
  G4_SrcRegRegion *insertCopyAtBBEntry(G4_BB *bb, G4_ExecSize newExecSize,
                                       G4_Operand *src);
  void broadcast(G4_BB *bb, INST_LIST_ITER it, int srcPos,
                 G4_SubReg_Align subAlign);

  G4_INST *splitInstWithByteDst(G4_INST *expand_op);
  G4_SubReg_Align getDclAlignment(int opndBytes, G4_INST *inst, bool isScalar);

  // HW conformity check functions
  void fixPackedSource(INST_LIST_ITER it, G4_BB *bb);
  bool fixMathInst(INST_LIST_ITER it, G4_BB *bb);
  bool fixMULInst(INST_LIST_ITER &it, G4_BB *bb);
  void fixMULHInst(INST_LIST_ITER &i, G4_BB *bb);
  void fixOpnds(INST_LIST_ITER it, G4_BB *bb, G4_Type &exType);
  bool fixLine(INST_LIST_ITER it, G4_BB *bb);
  bool fixOpndType(INST_LIST_ITER it, G4_BB *bb);
  void fixPackedHFConversions(INST_LIST_ITER it, G4_BB *bb);

  bool fixIndirectMoviSimd16ToSimd8(INST_LIST_ITER i, G4_BB *bb);
  bool fixIndirectOpnd(INST_LIST_ITER i, G4_BB *bb);
  bool fixIndirectSrcForCompressedInst(INST_LIST_ITER i, G4_BB *bb);
  void fix3SrcInst(INST_LIST_ITER i, G4_BB *bb);
  void fixAlign13SrcInst(INST_LIST_ITER i, G4_BB *bb);
  void fixCompareInst(INST_LIST_ITER i, G4_BB *bb, G4_Type exType,
                      int dst_elsize);
  bool fixDstAlignment(INST_LIST_ITER i, G4_BB *bb, G4_Type extype,
                       unsigned int dst_elsize);
  void fixPredicateIndirectInst(INST_LIST_ITER i, G4_BB *bb);
  bool fixDstAlignmentWithVectorImm(INST_LIST_ITER i, G4_BB *bb);
  bool fixAcc(INST_LIST_ITER i, G4_BB *bb);
  void fixDstHstride(INST_LIST_ITER i, int extypesize);
  void fixMADInst(G4_BB *bb);
  void fixBfnInst(G4_BB *bb);
  void fixSrcRegion(G4_INST *inst);
  bool fixOddAlignSrc1Region(INST_LIST_ITER i, G4_BB *bb);
  void conformBB(G4_BB *bb);
  void fixSADA2Inst(G4_BB *bb);
  void fixMixedHFInst(G4_BB *bb);
  void fixSendInst(G4_BB *bb);
  void fixsrc1src2Overlap(G4_BB *bb);
  bool canSplitByteDst(G4_opcode op);
  bool fixInstOpndTypeAlign(INST_LIST_ITER i, G4_BB *bb);
  void fixOpndTypeAlign(G4_BB *bb);
  void fixInstExecSize(G4_BB *bb);
  bool reduceExecSize(INST_LIST_ITER iter, G4_BB *bb);
  bool reduceExecSizeForMath(INST_LIST_ITER iter, G4_BB *bb);
  bool checkSrcDstOverlap(INST_LIST_ITER iter, G4_BB *bb, bool compOpt);
  void splitInstruction(INST_LIST_ITER iter, G4_BB *bb, bool compOpt,
                        uint8_t numInFirstMov, bool rule4_11,
                        bool allowSrcCrossGRF);
  void splitSIMD32Inst(INST_LIST_ITER iter, G4_BB *bb);
  void moveSrcToGRF(INST_LIST_ITER it, uint32_t srcNum, uint16_t numGRF,
                    G4_BB *bb);
  void saveDst(INST_LIST_ITER &it, uint8_t stride, G4_BB *bb);
  void restoreDst(INST_LIST_ITER &it, G4_DstRegRegion *origDst, G4_BB *bb);
  void insertMovAfter(INST_LIST_ITER &it, uint16_t stride, G4_BB *bb);
  void removeBadSrc(INST_LIST_ITER &it, G4_BB *bb, bool crossGRFDst,
                    bool oneGRFSrc[3], bool badTwoGRFSrc[3]);
  uint8_t checkMinExecSize(G4_opcode op);
  void convertMAD2MulAdd(INST_LIST_ITER iter, G4_BB *bb);
  void maintainDU4TempMov(G4_INST *inst, G4_INST *movInst);
  void fixImm64(INST_LIST_ITER i, G4_BB *bb);
  bool checkSrcCrossGRF(INST_LIST_ITER &i, G4_BB *bb);
  G4_INST *checkSrcDefInst(G4_INST *inst, G4_INST *def_inst, uint32_t srcNum);
  bool emulate64bMov(INST_LIST_ITER iter, G4_BB *bb);
  bool fix64bInst(INST_LIST_ITER i, G4_BB *bb);
  bool fixPlaneInst(INST_LIST_ITER i, G4_BB *bb);
  void expandPlaneInst(INST_LIST_ITER i, G4_BB *bb);
  bool fixAddcSubb(G4_BB *bb);
  void fixDataLayout();
  void fixBFMixedMode();
  bool fixMov(INST_LIST_ITER i, G4_BB *bb);
  bool fixRotate(INST_LIST_ITER i, G4_BB *bb);
  bool fixIntToHFMove(G4_BB *bb);

  bool isFloatOr64b(G4_INST *inst);
  uint16_t getSrcStride(G4_SrcRegRegion *src);
  bool fixBFMove(INST_LIST_ITER i, G4_BB *bb);
  void fixUnalignedRegions(INST_LIST_ITER it, G4_BB *bb);
  bool fixFcvt(INST_LIST_ITER i, G4_BB *bb);
  void fixByteXBarRestriction(INST_LIST_ITER it, G4_BB *bb);
  void fixDPAS(INST_LIST_ITER it, G4_BB *bb);
  bool fixSrnd(INST_LIST_ITER i, G4_BB *bb);
  void fixShiftInsts(INST_LIST_ITER i, G4_BB *bb);
  void split64bCopyToSIMD1Insts(INST_LIST_ITER it, G4_BB *bb);

  void helperGenerateTempDst(G4_BB *bb, INST_LIST_ITER instIter, G4_INST *inst,
                             uint8_t hStride, G4_Type tempDstType,
                             G4_SubReg_Align subAlign = Any);

  bool isGoodAlign16Src(G4_INST *inst, int srcPos);
  bool isGoodAlign1TernarySrc(G4_INST *inst, int srcPos, bool canBeImm);
  bool isGoodAlign1TernaryDst(G4_INST *inst) const;
  void copyDwords(G4_Declare *dst, int dstOffset, G4_Declare *src,
                  int srcOffset, int numDwords, G4_BB *bb, INST_LIST_ITER iter);

  void copyDwordsIndirect(G4_Declare *dst, G4_SrcRegRegion *src, int numDwords,
                          G4_BB *bb, INST_LIST_ITER iter);

  void copyRegs(G4_Declare *dst, int dstOffset, G4_Declare *src, int srcOffset,
                int numRegs, G4_BB *bb, INST_LIST_ITER iter);

  bool isFpMadPreferred(G4_BB *bb, INST_LIST_ITER iter);
  bool generateFPMad(G4_BB *bb, INST_LIST_ITER iter);
  bool generateAlign1Mad(G4_BB *bb, INST_LIST_ITER iter);
  bool hasSameSubregOffset(G4_INST *inst) const;
  bool hasSameSubregOffset(G4_INST *inst, uint32_t &byteOffset) const;

  void fixImmAndARFSrc(INST_LIST_ITER it, G4_BB *bb);
  void generateMacl(INST_LIST_ITER it, G4_BB *bb);
  void doGenerateMacl(INST_LIST_ITER it, G4_BB *bb);

  void fixSelCsel(INST_LIST_ITER it, G4_BB *bb);

  void avoidDstSrcOverlap(PointsToAnalysis &p);
  void avoidInstDstSrcOverlap(INST_LIST_ITER it, G4_BB *bb,
                              PointsToAnalysis &p);

  void replaceHFBFwithFloat(INST_LIST_ITER it, G4_BB *bb);

  bool hasDPASSourceTwoReuse(DPASSrc2RSCache *src2GRFCache, G4_INST *inst);

  bool checkSrcMod(INST_LIST_ITER it, G4_BB *bb, int srcPos);

  void fixSrc2(INST_LIST_ITER it, G4_BB *bb, bool swapSrc0and2);

  void fixVxHFloat64b(INST_LIST_ITER it, G4_BB *bb);

  void fixPredCtrl(INST_LIST_ITER it, G4_BB *bb);

  // Calla src register must be grf aligned (sub-reg offset must be 0)
  void fixCalla(INST_LIST_ITER it, G4_BB *bb);

  // If alignment and region of all operands of any instruction are conformed
  // by a dedicated function, return true.
  // This is used to skip generic conformity functions, such as
  // fixOpndTypeAlign().
  bool hasDedicateAlignRegionConformity(INST_LIST_ITER it) const {
    return hasDedicateAlignRegionConformity(*it);
  }
  bool hasDedicateAlignRegionConformity(const G4_INST *I) const;

  void fixSrc1Region(INST_LIST_ITER it, G4_BB *bb);

  INST_LIST_ITER fixMadwInst(INST_LIST_ITER i, G4_BB *bb);

  void fixFloatARFDst(INST_LIST_ITER it, G4_BB *bb);

  void fixImmAddrOffsetOOB(INST_LIST_ITER it, G4_BB *bb);

protected:
  G4_DstRegRegion *insertMovAfter(INST_LIST_ITER &it, G4_DstRegRegion *dst,
                                  G4_Type type, G4_BB *bb,
                                  G4_SubReg_Align dstAlign = Any);
  G4_Operand *insertMovBefore(INST_LIST_ITER it, uint32_t srcNum, G4_Type type,
                              G4_BB *bb, G4_SubReg_Align tmpAlign = Any);
  G4_Operand *insertMovBefore(INST_LIST_ITER it, uint32_t srcNum, G4_Type type,
                              G4_BB *bb, uint16_t stride,
                              G4_SubReg_Align tmpAlign = Any);

  // replace src <srcNum> for inst <*it> with a temp variable of type <type>
  // This is used to satisfy various HW restrictions on src
  // type/alignment/region/modifier/etc.
  void replaceSrc(INST_LIST_ITER it, uint32_t srcNum, G4_Type type, G4_BB *bb,
                  G4_SubReg_Align tmpAlign = Any) {
    G4_INST *inst = *it;
    inst->setSrc(insertMovBefore(it, srcNum, type, bb, tmpAlign), srcNum);
  }
  // replace dst for inst <*it> with a temp variable of type <type>
  // the original dst is now the dst of a new move instruction from the temp
  // variable. This is used to satisfy various HW restrictions on dst
  // type/alignment/etc.
  void replaceDst(INST_LIST_ITER it, G4_Type type,
                  G4_SubReg_Align dstAlign = Any) {
    G4_INST *inst = *it;
    inst->setDest(insertMovAfter(it, inst->getDst(), type, curBB, dstAlign));
  }

public:
  HWConformity(IR_Builder &b, G4_Kernel &k) : builder(b), kernel(k) {}
  void chkHWConformity();
  static void tryEliminateMadSrcModifier(IR_Builder &builder, G4_INST *inst);
  bool checkDPASSrcDstOverlap(INST_LIST_ITER iter, G4_BB *bb);
  G4_INST *evenlySplitDPAS8x8Inst(INST_LIST_ITER iter, G4_BB *bb);
  void DPASWA(G4_BB *bb, DPASSrc2RSCache *src2GRFCache);
  void localizeForAcc(G4_BB *bb);
  void splitDWMULInst(INST_LIST_ITER &start, INST_LIST_ITER &end, G4_BB *bb);
  void fixMulSrc1(INST_LIST_ITER i, G4_BB *bb);
  bool evenlySplitInst(INST_LIST_ITER iter, G4_BB *bb,
                       bool checkOverlap = true);
};
} // namespace vISA
// single entry point for HW conformity checks
extern void HWConformityChk(vISA::IR_Builder &builder, vISA::G4_Kernel &kernel);

#endif /* _HWCONFORMITY_H_ */
