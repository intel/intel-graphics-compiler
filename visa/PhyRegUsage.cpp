/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PhyRegUsage.h"
#include "FlowGraph.h"
#include <optional>
#include "GraphColor.h"

using namespace vISA;

PhyRegUsage::PhyRegUsage(PhyRegAllocationState &p, FreePhyRegs &pFPR)
    : gra(p.gra), lrs(p.lrs), colorHeuristic(FIRST_FIT), FPR(pFPR), AS(p),
      totalGRFNum(p.totalGRF), honorBankBias(p.doBankConflict),
      avoidBundleConflict(p.doBundleConflict),
      builder(*p.gra.kernel.fg.builder), regPool(p.gra.regPool) {
  maxGRFCanBeUsed = p.maxGRFCanBeUsed;
  regFile = p.rFile;
  overlapTest = false;

  // Make all physical registers free at the beginning.
  FPR.reset();
}

void PhyRegUsage::markBusyForDclSplit(G4_RegFileKind kind, unsigned regNum,
                                      unsigned regOff,
                                      unsigned nunits, // word units
                                      unsigned numRows) {
  vISA_ASSERT(numRows > 0 && nunits > 0, ERROR_INTERNAL_ARGUMENT);
  vISA_ASSERT(regNum + numRows <= maxGRFCanBeUsed, ERROR_UNKNOWN);

  unsigned uwordsPerGRF = builder.numEltPerGRF<Type_UW>();
  unsigned start_GRF = (regNum * uwordsPerGRF + regOff) / uwordsPerGRF;
  unsigned end_GRF = (regNum * uwordsPerGRF + regOff + nunits) / uwordsPerGRF;

  unsigned start_sub_GRF = (regNum * uwordsPerGRF + regOff) % uwordsPerGRF;
  unsigned end_sub_GRF =
      (regNum * uwordsPerGRF + regOff + nunits) % uwordsPerGRF;

  for (unsigned i = start_GRF; i < end_GRF; i++) {
    FPR.availableGregs[i] = false;
    // FIXME: I think it should be 0 regardless, the upper half does not exist
    // for 32 byte GRF platforms anyway.
    FPR.availableSubRegs[i] = builder.getGRFSize() == 64 ? 0 : 0xFFFF0000;
  }

  if (end_sub_GRF) {
    FPR.availableGregs[end_GRF] = false;
    if (start_GRF == end_GRF) {
      auto subregMask = getSubregBitMask(start_sub_GRF, nunits);
      FPR.availableSubRegs[end_GRF] &= ~subregMask;
    } else {
      auto subregMask = getSubregBitMask(0, end_sub_GRF);
      FPR.availableSubRegs[end_GRF] &= ~subregMask;
    }
  }
}
//
// mark availRegs[start ... start+num-1] free again
//
void PhyRegUsage::freeContiguous(bool availRegs[], unsigned start, unsigned num,
                                 unsigned maxRegs) {
  for (unsigned i = start; i < start + num; i++) {
    vISA_ASSERT(i < maxRegs && availRegs[i] == false, ERROR_UNKNOWN);
    availRegs[i] = true;
  }
}
//
// mark sub reg [regOff .. regOff + nbytes -1] of the reg regNum free
//
void PhyRegUsage::freeGRFSubReg(unsigned regNum, unsigned regOff,
                                unsigned nwords, G4_Type ty) {
  //
  // adjust regOff to its corresponding word position
  //

  int startWord = regOff * TypeSize(ty) / G4_WSIZE;
  auto subregMask = getSubregBitMask(startWord, nwords);
  FPR.availableSubRegs[regNum] |= subregMask;

  //
  // if all sub regs of regNum are free, then unlink the reg
  //
  if (FPR.availableSubRegs[regNum] == 0xFFFFFFFF) {
    vISA_ASSERT(!FPR.availableGregs[regNum], ERROR_UNKNOWN);
    FPR.availableGregs[regNum] = true;
  }
}

//
// free registers that are held by intv
//
void PhyRegUsage::freeRegs(LiveRange *varBasis) {
  G4_Declare *decl = varBasis->getDcl();
  G4_RegFileKind kind = decl->getRegFile();
  vISA_ASSERT(varBasis->getPhyReg(), ERROR_UNKNOWN);
  if (decl->useGRF()) {
    vISA_ASSERT(varBasis->getPhyReg()->isGreg(), ERROR_UNKNOWN);
    if (canGRFSubRegAlloc(decl)) {
      freeGRFSubReg(((G4_Greg *)varBasis->getPhyReg())->getRegNum(),
                    varBasis->getPhyRegOff(),
                    numAllocUnit(decl->getNumElems(), decl->getElemType()),
                    decl->getElemType());
    } else {
      freeContiguous(FPR.availableGregs,
                     ((G4_Greg *)varBasis->getPhyReg())->getRegNum(),
                     decl->getNumRows(), totalGRFNum);
    }
  } else if (kind == G4_ADDRESS) {
    vISA_ASSERT(varBasis->getPhyReg()->isAreg(), ERROR_UNKNOWN);
    freeContiguous(FPR.availableAddrs, varBasis->getPhyRegOff(),
                   numAllocUnit(decl->getNumElems(), decl->getElemType()),
                   builder.getNumAddrRegisters());
  } else if (kind == G4_FLAG) {
    vISA_ASSERT(varBasis->getPhyReg()->isFlag(), ERROR_UNKNOWN);
    freeContiguous(FPR.availableFlags, varBasis->getPhyRegOff(),
                   numAllocUnit(decl->getNumElems(), decl->getElemType()),
                   builder.getNumFlagRegisters());
  } else // not yet handled
    vISA_ASSERT(false, ERROR_UNKNOWN);
}

static int getSubAlignInWords(G4_SubReg_Align subAlign) {
  return static_cast<int>(subAlign);
}

unsigned short PhyRegUsage::getOccupiedBundle(const G4_Declare *dcl) const {
  unsigned short occupiedBundles = 0;
  unsigned bundleNum = 0;
  if (!avoidBundleConflict) {
    return occupiedBundles;
  }

  if (!builder.hasDPAS() ||
      !builder.getOption(vISA_EnableDPASBundleConflictReduction)) {
    return 0;
  }

  for (const BundleConflict &conflict : gra.getBundleConflicts(dcl)) {
    unsigned reg = -1;
    int offset = 0;

    offset = conflict.offset;
    const G4_RegVar *regVar = conflict.dcl->getRegVar();
    if (regVar->isPhyRegAssigned() && regVar->isGreg()) {
      reg = regVar->getPhyReg()->asGreg()->getRegNum();
    } else if (regVar->getId() < lrs.size()) {
      LiveRange *lr = lrs[regVar->getId()];
      if (lr && lr->getPhyReg() && lr->getPhyReg()->isGreg()) {
        reg = lr->getPhyReg()->asGreg()->getRegNum();
      }
    }

    if (reg != -1) {
      unsigned bundle = gra.get_bundle(reg, offset);
      unsigned bundle1 = gra.get_bundle(reg, offset + 1);
      if (!(occupiedBundles & ((unsigned short)1 << bundle))) {
        bundleNum++;
      }
      occupiedBundles |= (unsigned short)1 << bundle;
      occupiedBundles |= (unsigned short)1 << bundle1;
    }
  }
  if (bundleNum > 12) {
    occupiedBundles = 0;
  }

  return occupiedBundles;
}

// returns the starting word index if we find enough free contiguous words
// satisfying alignment, -1 otherwise
int PhyRegUsage::findContiguousWords(uint32_t words, G4_SubReg_Align subAlign,
                                     int numWords) const {
  // early exit in (false?) hope of saving compile time
  if (words == 0) {
    return -1;
  }

  int step = getSubAlignInWords(subAlign);
  int startWord = 0;
  int uwordsPerGRF = builder.numEltPerGRF<Type_UW>();

  for (int i = startWord; i + numWords <= uwordsPerGRF; i += step) {
    uint32_t bitMask = getSubregBitMask(i, numWords);
    if ((bitMask & words) == bitMask) {
      return i;
    }
  }

  return -1;
}

//
// look for contiguous available regs starting from startPos
//
bool PhyRegUsage::findContiguousGRF(bool availRegs[], const BitSet *forbidden,
                                    unsigned occupiedBundles, BankAlign align,
                                    unsigned numRegNeeded, unsigned maxRegs,
                                    unsigned &startPos, unsigned &idx,
                                    bool forceCalleeSaveOnly, bool isEOTSrc) {
  unsigned startPosRunOne = startPos;
  unsigned endPosRunOne = maxRegs;

  if (isEOTSrc && (startPosRunOne >= maxRegs)) {
    return false;
  } else {
    vISA_ASSERT(startPosRunOne < maxRegs, ERROR_UNKNOWN);
  }
  bool found =
      findContiguousNoWrapGRF(availRegs, forbidden, occupiedBundles, align,
                              numRegNeeded, startPosRunOne, endPosRunOne, idx);

  if (startPosRunOne > 0 && found == false && !isEOTSrc &&
      !forceCalleeSaveOnly) {
    unsigned startPosRunTwo = 0;
    unsigned endPosRunTwo = startPos + numRegNeeded;
    endPosRunTwo = std::min(endPosRunTwo, maxRegs);
    vISA_ASSERT(endPosRunTwo > 0 && endPosRunTwo <= maxRegs, ERROR_UNKNOWN);
    found = findContiguousNoWrapGRF(availRegs, forbidden, occupiedBundles,
                                    align, numRegNeeded, startPosRunTwo,
                                    endPosRunTwo, idx);
  }

  if (found) {
    vISA_ASSERT(idx < maxRegs && idx + numRegNeeded <= maxRegs, ERROR_UNKNOWN);

    if (colorHeuristic == ROUND_ROBIN || builder.getOption(vISA_GCRRInFF)) {
      // Set start postion of next variable according to round robin algrithm in
      // case the next variable needs use round robin algorithm. For
      // vISA_GCRRInFF, if the next variable uses first fit algorithm,
      // assignColors will re-assign 0 to the variable before this function is
      // executed.
      startPos = (idx + numRegNeeded) % maxRegs;
    }
  }

  return found;
}

//
// look for contiguous available regs starting from startPos
//
bool PhyRegUsage::findContiguousAddrFlag(
    bool availRegs[], const BitSet *forbidden, G4_SubReg_Align subAlign,
    unsigned numRegNeeded, unsigned maxRegs, unsigned &startPos, unsigned &idx,
    bool isCalleeSaveBias, bool isEOTSrc) {
  unsigned startPosRunOne = startPos;
  unsigned endPosRunOne = maxRegs;

  if (isEOTSrc && (startPosRunOne >= maxRegs)) {
    return false;
  } else {
    vISA_ASSERT(startPosRunOne < maxRegs, ERROR_UNKNOWN);
  }
  bool found =
      findContiguousNoWrapAddrFlag(availRegs, forbidden, subAlign, numRegNeeded,
                                   startPosRunOne, endPosRunOne, idx);

  if (startPosRunOne > 0 && found == false && !isEOTSrc && !isCalleeSaveBias) {
    unsigned startPosRunTwo = 0;
    unsigned endPosRunTwo = startPos + numRegNeeded;
    endPosRunTwo = std::min(endPosRunTwo, maxRegs);
    vISA_ASSERT(endPosRunTwo > 0 && endPosRunTwo <= maxRegs, ERROR_UNKNOWN);
    found = findContiguousNoWrapAddrFlag(availRegs, forbidden, subAlign,
                                         numRegNeeded, startPosRunTwo,
                                         endPosRunTwo, idx);
  }

  if (found) {
    vISA_ASSERT(idx < maxRegs && idx + numRegNeeded <= maxRegs, ERROR_UNKNOWN);

    if (colorHeuristic == ROUND_ROBIN) {
      startPos = (idx + numRegNeeded) % maxRegs;
    }
  }

  return found;
}

bool PhyRegUsage::findContiguousGRFFromBanks(
    G4_Declare *dcl, bool availRegs[], const BitSet *forbidden,
    BankAlign origAlign, unsigned &idx,
    bool oneGRFBankDivision) { // EOT is not handled in this function
  bool found = false;
  unsigned numRegNeeded = dcl->getNumRows();
  auto dclBC = gra.getBankConflict(dcl);
  bool gotoSecondBank = (dclBC == BANK_CONFLICT_SECOND_HALF_EVEN ||
                         dclBC == BANK_CONFLICT_SECOND_HALF_ODD) &&
                        (dcl->getNumRows() > 1);

  BankAlign align = origAlign;
  if (dclBC != BANK_CONFLICT_NONE && align == BankAlign::Either &&
      dcl->getNumRows() <= 1) {
    align = gra.getBankAlign(dcl);
  }

  vISA_ASSERT(AS.bank1_end < totalGRFNum && AS.bank1_start < totalGRFNum &&
                  AS.bank2_start < totalGRFNum && AS.bank2_end < totalGRFNum,
              "Wrong bank boundaries value");

  if (colorHeuristic == ROUND_ROBIN) {
    // For round robin, AS.bank1_end and AS.bank2_end are fixed.
    if (gotoSecondBank) // For odd aligned varaibe, we put them to a specific
                        // sections.
    {
      // From maxGRFCanBeUsed - 1 to AS.bank2_end
      vISA_ASSERT(AS.bank2_start >= AS.bank2_end,
                  "Second bank's start can not less than end\n");

      if ((AS.bank2_start - AS.bank2_end + 1) >= numRegNeeded) // 3 - 2 + 1 >= 2
      {
        found = findFreeRegs(availRegs, forbidden, align, numRegNeeded,
                             AS.bank2_start, AS.bank2_end, idx, gotoSecondBank,
                             oneGRFBankDivision);
      }

      if (!found) {
        if (maxGRFCanBeUsed - 1 >= AS.bank2_start + numRegNeeded) {
          found = findFreeRegs(availRegs, forbidden, align, numRegNeeded,
                               maxGRFCanBeUsed - 1, AS.bank2_start + 1, idx,
                               gotoSecondBank, oneGRFBankDivision);
        } else {
          return false;
        }
      }

      if (found) {
        AS.bank2_start = idx - 1;
        if (AS.bank2_start < AS.bank2_end) {
          AS.bank2_start = maxGRFCanBeUsed - 1;
        }
      }
    } else { // From 0 to AS.bank1_end
      if (AS.bank1_end - AS.bank1_start + 1 >= numRegNeeded) {
        found = findFreeRegs(availRegs, forbidden, BankAlign::Even,
                             numRegNeeded, AS.bank1_start, AS.bank1_end, idx,
                             gotoSecondBank, oneGRFBankDivision);
      }

      if (!found) {
        if (AS.bank1_start >= numRegNeeded) {
          found =
              findFreeRegs(availRegs, forbidden, BankAlign::Even, numRegNeeded,
                           0, AS.bank1_start - 2 + numRegNeeded, idx,
                           gotoSecondBank, oneGRFBankDivision);
        }
      }

      if (found) {
        AS.bank1_start = idx + numRegNeeded;
        if (AS.bank1_start > AS.bank1_end) {
          AS.bank1_start = 0;
        }
      }
    }
  } else {
    // For first fit, the AS.bank1_start and AS.bank2_start are fixed.
    // AS.bank2_end and AS.bank1_end are dynamically decided, but can not change
    // in one direction (MIN or MAX).
    if (gotoSecondBank) // For odd aligned varaibe, we put them to a specific
                        // sections.
    {
      found = findFreeRegs(availRegs, forbidden, align, numRegNeeded,
                           maxGRFCanBeUsed - 1, 0, idx, gotoSecondBank,
                           oneGRFBankDivision);

      if (found) {
        AS.bank2_end = std::min(idx, AS.bank2_end);
      }
    } else {
      found = findFreeRegs(availRegs, forbidden, align, numRegNeeded, 0,
                           maxGRFCanBeUsed - 1, idx, gotoSecondBank,
                           oneGRFBankDivision);
      if (found) {
        AS.bank1_end = std::max(idx + numRegNeeded - 1, AS.bank1_end);
      }
    }

    if (AS.bank2_end <= AS.bank1_end) {
      found = false;
    }
  }

  return found;
}

bool PhyRegUsage::isOverlapValid(unsigned int reg, unsigned int numRegs) {
  for (unsigned int i = reg; i < (reg + numRegs); i++) {
    auto k = getWeakEdgeUse(i);
    if (!(k == 0 || k == (i - reg + 1))) {
      // This condition will be taken when there is a partial
      // overlap.
      return false;
    }
  }

  return true;
}

//
// look for contiguous available regs from startPos to maxRegs
//
bool PhyRegUsage::findContiguousNoWrapGRF(
    bool availRegs[], const BitSet *forbidden, unsigned short occupiedBundles,
    BankAlign align, unsigned numRegNeeded, unsigned startPos, unsigned endPos,
    unsigned &idx) {
  unsigned i = startPos;
  while (i < endPos) {
    if (((i & 0x1) &&
         align ==
             BankAlign::Even) || // i is odd but intv needs to be even aligned
        ((i & 0x1) == 0 &&
         align == BankAlign::Odd) || // i is even but intv needs to be odd aligned
        ((i % 4) != 0 &&
         align == BankAlign::QuadGRF)) // check for 4GRF alignment
    {
      i++;
    } else {
      if (align == BankAlign::Even2GRF) {
        while ((i % 4 >= 2) || ((numRegNeeded >= 2) && (i % 2 != 0))) {
          i++;
        }
      } else if (align == BankAlign::Odd2GRF) {
        while ((i % 4 < 2) || ((numRegNeeded >= 2) && (i % 2 != 0))) {
          i++;
        }
      }

      if (numRegNeeded == 0 || i + numRegNeeded > endPos)
        return false; // no available regs
      //
      // find contiguous numRegNeeded registers
      // forbidden != NULL then check forbidden
      //
      unsigned j = i;
      if (overlapTest && !isOverlapValid(i, numRegNeeded)) {
        i++;
      } else if (occupiedBundles & (1 << gra.get_bundle(i, 0)) ||
                 occupiedBundles & (1 << gra.get_bundle(i, 1))) {
        i++;
      } else {
        for (; j < i + numRegNeeded && availRegs[j] && (!forbidden || !forbidden->isSet(j));
             j++)
          ;
        if (j == i + numRegNeeded) {
          for (unsigned k = i; k < j; k++)
            availRegs[k] = false;
          idx = i;
          return true;
        } else
          i = j + 1;
      }
    }
  }
  return false; // no available regs
}

//
// look for contiguous available regs from startPos to maxRegs
//
bool PhyRegUsage::findContiguousNoWrapAddrFlag(
    bool availRegs[], const BitSet *forbidden, G4_SubReg_Align subAlign,
    unsigned numRegNeeded, unsigned startPos, unsigned endPos, unsigned &idx) {
  unsigned i = startPos;
  while (i < endPos) {
    //
    // some register assignments need special alignment, we check
    // whether the alignment criteria is met.
    //
    if (endPos <= 16 && subAlign == Sixteen_Word && i != 0) {
      // for addr-reg, Sixteen_Word sub - align should have i = 0
      return false;
    } else if ((subAlign == ThirtyTwo_Word && i % 32 != 0) || // 32_word align
               (subAlign == Sixteen_Word &&
                i % 16 != 0) ||                          // 16_word align check
               (subAlign == Eight_Word && i % 8 != 0) || // 8_Word align check
               (i & 0x1 && subAlign == Even_Word) ||     // even aligned check
               (subAlign == Four_Word && (i % 4 != 0)))  // 4_word alignment
      i++;
    else {
      if (numRegNeeded == 0 || i + numRegNeeded > endPos) {
        return false; // no available regs
      }
      //
      // find contiguous numRegNeeded registers
      // forbidden != NULL then check forbidden
      //
      unsigned j = i;
      for (; j < i + numRegNeeded && availRegs[j] &&
             (!forbidden || !forbidden->isSet(j));
           j++)
        ;
      if (j == i + numRegNeeded) {
        for (unsigned k = i; k < j; k++)
          availRegs[k] = false;
        idx = i;
        return true;
      } else {
        i = j + 1;
      }
    }
  }
  return false; // no available regs
}

bool PhyRegUsage::findFreeRegs(
    bool availRegs[], const BitSet *forbidden, BankAlign align,
    unsigned numRegNeeded,
    unsigned startRegNum, // inclusive
    unsigned
        endRegNum, // inclusive: less and equal when startRegNum <= endRegNum,
                   // larger and equal when startRegNum > endRegNum
    unsigned &idx, bool gotoSecondBank, bool oneGRFBankDivision) {
  bool forward = startRegNum <= endRegNum ? true : false;
  int startReg = forward ? startRegNum : startRegNum - numRegNeeded + 1;
  int endReg = forward ? endRegNum - numRegNeeded + 1 : endRegNum;
  int i = startReg;

  while (1) {
    if (forward) {
      if (i > endReg)
        break;
    } else {
      if (i < endReg)
        break;
    }

    if ((align == BankAlign::Even2GRF) && (i % 2 != 0 || i % 4 == 3)) {
      i += forward ? 1 : -1;
      continue;
    } else if ((align == BankAlign::Odd2GRF) && (i % 2 != 0 || i % 4 == 1)) {
      i += forward ? 1 : -1;
      continue;
    } else if ((((i & 0x1) &&
                 align == BankAlign::Even) || // i is odd but intv needs to be
                                              // even aligned
                (((i & 0x1) == 0) &&
                 (align == BankAlign::Odd)))) // i is even but intv needs to be
                                              // odd aligned
    {
      i += forward ? 1 : -1;
      continue;
    } else {
      if ((forward && (i > endReg)) || (!forward && (i < endReg))) {
        return false; // no available regs
      }

      if (regFile == G4_GRF && overlapTest &&
          !isOverlapValid(i, numRegNeeded)) {
        i += forward ? 1 : -1;
      } else {
        // find contiguous numRegNeeded registers
        // forbidden != NULL then check forbidden
        //
        unsigned j = i;
        for (; j < i + numRegNeeded && availRegs[j] &&
               (!forbidden || !forbidden->isSet(j));
             j++)
          ;
        if (j == i + numRegNeeded) {
          for (unsigned k = i; k < j; k++)
            availRegs[k] = false;
          idx = i;
          return true;
        } else { // Jump over the register region which a poke in the end
          if (forward) {
            i = j + 1;
          } else {
            if (j > numRegNeeded) {
              i = j - numRegNeeded;
            } else {
              break;
            }
          }
        }
      }
    }
  }

  return false;
}

//
// return true, if the var can be allocated using sub reg
//
bool PhyRegUsage::canGRFSubRegAlloc(G4_Declare *decl) {
  if (decl->getNumRows() != 1) // more than 1 row
    return false;
  if (numAllocUnit(decl->getNumElems(), decl->getElemType()) <
      builder.numEltPerGRF<Type_UW>())
    return true;
  return false;
}

void PhyRegUsage::findGRFSubRegFromRegs(int startReg, int endReg, int step,
                                        PhyReg *phyReg,
                                        G4_SubReg_Align subAlign,
                                        unsigned nwords, const BitSet *forbidden,
                                        bool fromPartialOccupiedReg) {
  int idx = startReg;
  while (1) {
    if (step > 0) {
      if (idx > endReg) {
        break;
      }
    } else {
      if (idx < endReg) {
        break;
      }
    }

    if (forbidden && forbidden->isSet(idx)) {
      idx += step;
      continue;
    }

    if (fromPartialOccupiedReg && FPR.availableSubRegs[idx] == 0xFFFFFFFF) {
      // favor partially allocated GRF first
      idx += step;
      continue;
    }

    int subreg =
        findContiguousWords(FPR.availableSubRegs[idx], subAlign, nwords);
    if (subreg != -1) {
      phyReg->reg = idx;
      phyReg->subreg = subreg;
      return;
    }

    idx += step;
  }

  return;
}

PhyRegUsage::PhyReg
PhyRegUsage::findGRFSubRegFromBanks(G4_Declare *dcl, const BitSet *forbidden,
                                    bool oneGRFBankDivision) {
  int startReg = 0, endReg = totalGRFNum;
  int step = 0;
  G4_SubReg_Align subAlign = gra.getSubRegAlign(dcl);
  unsigned nwords = numAllocUnit(dcl->getNumElems(), dcl->getElemType());
  auto dclBC = gra.getBankConflict(dcl);
  bool gotoSecondBank = dclBC == BANK_CONFLICT_SECOND_HALF_EVEN ||
                        dclBC == BANK_CONFLICT_SECOND_HALF_ODD;

  if (gotoSecondBank && oneGRFBankDivision) {
    startReg = (maxGRFCanBeUsed - 1);
    startReg = startReg % 2 ? startReg : startReg - 1;
    if (colorHeuristic == ROUND_ROBIN) {
      endReg = AS.bank2_end;
    } else {
      endReg = 0;
    }
    step = -2;
  } else if (gotoSecondBank &&
             !oneGRFBankDivision) // We will depends on low high, treated as
                                  // even align
  {
    startReg = (maxGRFCanBeUsed - 1);
    startReg = startReg % 2 ? startReg - 1 : startReg;
    if (colorHeuristic == ROUND_ROBIN) {
      endReg = AS.bank2_end;
    } else {
      endReg = 0;
    }
    step = -1;
  } else {
    if (colorHeuristic == ROUND_ROBIN) {
      startReg = 0;
      endReg = AS.bank1_end;
    } else {
      startReg = 0;
      endReg = maxGRFCanBeUsed - 1;
    }
    if (oneGRFBankDivision) {
      step = 2;
    } else {
      step = 1;
    }
  }

  PhyReg phyReg = {-1, -1};

  // Try to find sub register from the registers which are partially occupied
  // already.
  findGRFSubRegFromRegs(startReg, endReg, step, &phyReg, subAlign, nwords,
                        forbidden, true);

  // If failed or across the boundary of specified bank, try again and find from
  // the registers which are totally free
  if (phyReg.reg == -1 ||
      (gotoSecondBank && ((unsigned)phyReg.reg <= AS.bank1_end)) ||
      (!gotoSecondBank && ((unsigned)phyReg.reg >= AS.bank2_end))) {
    findGRFSubRegFromRegs(startReg, endReg, step, &phyReg, subAlign, nwords,
                          forbidden, false);
  }

  if (phyReg.reg != -1 && colorHeuristic == FIRST_FIT) {
    if (gotoSecondBank) {
      AS.bank2_end = std::min((unsigned)phyReg.reg, AS.bank2_end);
    } else {
      AS.bank1_end = std::max((unsigned)phyReg.reg, AS.bank1_end);
    }
    if (AS.bank1_end >= AS.bank2_end) {
      phyReg.reg = -1;
    }
  }

  return phyReg;
}

//
// return reg and subRegOff (subr)
// To support sub-reg alignment
//
PhyRegUsage::PhyReg
PhyRegUsage::findGRFSubReg(const BitSet *forbidden, bool calleeSaveBias,
                           bool callerSaveBias, BankAlign align,
                           G4_SubReg_Align subAlign, unsigned nwords) {
  int startReg = 0, endReg = totalGRFNum;
  PhyReg phyReg = {-1, -1};
  if (builder.getOption(vISA_GCRRInFF)) {
    startReg = AS.startGRFReg;
  }

  if (calleeSaveBias) {
    startReg = builder.kernel.stackCall.calleeSaveStart();
  } else if (callerSaveBias) {
    endReg = builder.kernel.stackCall.calleeSaveStart();
  }
  int step = align == BankAlign::Even ? 2 : 1;

  auto findSubGRFAlloc = [step, forbidden, this, subAlign,
                          nwords](unsigned int startReg, unsigned int endReg) {
    PhyReg phyReg = {-1, -1};
    for (auto idx = startReg; idx < endReg; idx += step) {
      // forbidden GRF is not an allocation candidate
      if (forbidden && forbidden->isSet(idx)) {
        continue;
      }

      // check if entire GRF is available
      if (FPR.availableSubRegs[idx] == 0xFFFFFFFF) {
        if (phyReg.reg == -1) {
          // favor partially allocated GRF first so dont
          // return this assignment yet
          phyReg.reg = idx;
          phyReg.subreg = 0;
        }
        continue;
      }

      int subreg =
          findContiguousWords(FPR.availableSubRegs[idx], subAlign, nwords);
      if (subreg != -1) {
        phyReg.reg = idx;
        phyReg.subreg = subreg;
        return phyReg;
      }
    }

    // either return {-1, -1} or an allocation where entire GRF is available
    return phyReg;
  };

  if (callerSaveBias || calleeSaveBias) {
    // attempt bias based assignment first
    phyReg = findSubGRFAlloc(startReg, endReg);
    if (phyReg.subreg != -1)
      return phyReg;
  }

  // Find sub-GRF allocation throughout GRF file
  if (builder.getOption(vISA_GCRRInFF)) {
    phyReg = findSubGRFAlloc(startReg, totalGRFNum);
    if (phyReg.subreg == -1) {
      phyReg = findSubGRFAlloc(0, AS.startGRFReg);
    }
    if (phyReg.subreg != -1) {
      AS.startGRFReg = (phyReg.reg + 1) % totalGRFNum;
    }
  } else {
    phyReg = findSubGRFAlloc(0, totalGRFNum);
  }

  return phyReg;
}

bool PhyRegUsage::assignGRFRegsFromBanks(LiveRange *varBasis, BankAlign align,
                                         const BitSet *forbidden,
                                         ColorHeuristic heuristic,
                                         bool oneGRFBankDivision) {
  colorHeuristic = heuristic;
  G4_Declare *decl = varBasis->getDcl();

  //
  // if regs are allocated to intv, i is the reg number and off is the reg
  // offset for sub reg allocation
  //
  unsigned i = 0; // avail reg number

  //
  // determine if we need to do sub reg allcoation
  //
  if (canGRFSubRegAlloc(decl)) {
    bool retVal = false;

    PhyRegUsage::PhyReg phyReg =
        findGRFSubRegFromBanks(decl, forbidden, oneGRFBankDivision);
    if (phyReg.reg != -1) {
      // based on type, adjust sub reg off accordingly
      // word: stay the same, dword: *2, byte: /2
      // assign r_i.off
      varBasis->setPhyReg(regPool.getGreg(phyReg.reg),
                          phyReg.subreg * G4_WSIZE / decl->getElemSize());
      retVal = true;
    }

    return retVal;
  } else {
    bool success = false;
    if (varBasis->getEOTSrc() && builder.hasEOTGRFBinding()) {
      bool forceCalleeSaveAlloc = builder.kernel.fg.isPseudoVCEDcl(decl);
      AS.startGRFReg = totalGRFNum - 16;
      success = findContiguousGRF(
          FPR.availableGregs, forbidden, 0, align, decl->getNumRows(),
          maxGRFCanBeUsed, AS.startGRFReg, i, forceCalleeSaveAlloc, true);
    } else {
      success = findContiguousGRFFromBanks(decl, FPR.availableGregs, forbidden,
                                           align, i, oneGRFBankDivision);
    }

    if (success) {
      varBasis->setPhyReg(regPool.getGreg(i), 0);
    }

    return success;
  }

  return false;
}

//
// find registers for intv
// To support sub-reg alignment
//
bool PhyRegUsage::assignRegs(bool highInternalConflict, LiveRange *varBasis,
                             const BitSet *forbidden, BankAlign align,
                             G4_SubReg_Align subAlign, ColorHeuristic heuristic,
                             float spillCost) {
  colorHeuristic = heuristic;

  G4_Declare *decl = varBasis->getDcl();
  G4_RegFileKind kind = decl->getRegFile();
  BankAlign bankAlign = align;

  //
  // if regs are allocated to intv, i is the reg number and off is the reg
  // offset for sub reg allocation
  //
  unsigned i = 0; // avail reg number

  auto getAlignToUse = [this](BankAlign align, BankAlign bankAlign) {
    if (GlobalRA::useGenericAugAlign(builder.getPlatformGeneration()))
      return (align != BankAlign::Either ? align : bankAlign);
    else
      return (bankAlign != BankAlign::Either ? bankAlign : align);
  };

  if (kind == G4_GRF) // general register file
  {
    //
    // determine if we need to do sub reg allcoation
    //
    if (canGRFSubRegAlloc(decl)) {
      bool retVal = false;
      int oldStartGRFReg = AS.startGRFReg;
      BankConflict varBasisBC =
          gra.getBankConflict(varBasis->getVar()->asRegVar()->getDeclare());

      if (!builder.getOptions()->getuInt32Option(vISA_ReservedGRFNum) &&
          honorBankBias && varBasisBC != BANK_CONFLICT_NONE) {
        if (highInternalConflict) {
          switch (varBasisBC) {
          case BANK_CONFLICT_FIRST_HALF_EVEN:
          case BANK_CONFLICT_FIRST_HALF_ODD:
            AS.startGRFReg = 0;
            break;
          case BANK_CONFLICT_SECOND_HALF_EVEN:
          case BANK_CONFLICT_SECOND_HALF_ODD:
            AS.startGRFReg = 64;
            break;
          default:
            break;
          }
        } else {
          bankAlign =
              gra.getBankAlign(varBasis->getVar()->asRegVar()->getDeclare());
        }
      }

      // If the var is biased to receive a callee-bias, start at r60 and wrap
      // around. NOTE: We are assuming a first-fit strategy when a callee-bias
      // is present.
      if (varBasis->getCalleeSaveBias()) {
        AS.startGRFReg = 60;
      }

      PhyRegUsage::PhyReg phyReg = findGRFSubReg(
          forbidden, varBasis->getCalleeSaveBias(),
          varBasis->getCallerSaveBias(), getAlignToUse(align, bankAlign),
          subAlign, numAllocUnit(decl->getNumElems(), decl->getElemType()));
      if (phyReg.reg != -1) {
        // based on type, adjust sub reg off accordingly
        // word: stay the same, dword: *2, byte: /2
        // assign r_i.off
        varBasis->setPhyReg(regPool.getGreg(phyReg.reg),
                            phyReg.subreg * G4_WSIZE / decl->getElemSize());
        retVal = true;
      }

      if (varBasis->getCalleeSaveBias()) {
        AS.startGRFReg = oldStartGRFReg;
      }

      return retVal;
    } else {
      int oldStartGRFReg = AS.startGRFReg;
      unsigned endGRFReg = maxGRFCanBeUsed; // round-robin reg  start bias
      BankConflict varBasisBC =
          gra.getBankConflict(varBasis->getVar()->asRegVar()->getDeclare());

      if (!builder.getOptions()->getuInt32Option(vISA_ReservedGRFNum) &&
          honorBankBias && varBasisBC != BANK_CONFLICT_NONE) {
        if (highInternalConflict) {
          switch (varBasisBC) {
          case BANK_CONFLICT_FIRST_HALF_EVEN:
          case BANK_CONFLICT_FIRST_HALF_ODD:
            AS.startGRFReg = 0;
            break;
          case BANK_CONFLICT_SECOND_HALF_EVEN:
          case BANK_CONFLICT_SECOND_HALF_ODD:
            AS.startGRFReg = 64;
            break;
          default:
            break;
          }
        } else {
          bankAlign =
              gra.getBankAlign(varBasis->getVar()->asRegVar()->getDeclare());
        }
      }

      // If the var is biased to receive a callee-bias, start at r60 and wrap
      // around. NOTE: We are assuming a first-fit strategy when a callee-bias
      // is present.
      if (varBasis->getCalleeSaveBias()) {
        AS.startGRFReg = builder.kernel.stackCall.calleeSaveStart();
      }

      if (varBasis->getEOTSrc() && builder.hasEOTGRFBinding()) {
        AS.startGRFReg = totalGRFNum - 16;
      }

      bool forceCalleeSaveAlloc = builder.kernel.fg.isPseudoVCEDcl(decl);
      unsigned short occupiedBundles = getOccupiedBundle(decl);
      bool success = findContiguousGRF(
          FPR.availableGregs, forbidden, occupiedBundles,
          getAlignToUse(align, bankAlign), decl->getNumRows(), endGRFReg,
          AS.startGRFReg, i, forceCalleeSaveAlloc, varBasis->getEOTSrc());
      if (!success && occupiedBundles) {
        success = findContiguousGRF(
            FPR.availableGregs, forbidden, 0,
            getAlignToUse(align, bankAlign), decl->getNumRows(), endGRFReg,
            AS.startGRFReg, i, forceCalleeSaveAlloc, varBasis->getEOTSrc());
      }
      if (success) {
        varBasis->setPhyReg(regPool.getGreg(i), 0);
      }

      if (varBasis->getEOTSrc()) {
        AS.startGRFReg = oldStartGRFReg;
      }

      if (varBasis->getCalleeSaveBias()) {
        AS.startGRFReg = oldStartGRFReg;
      }

      return success;
    }
  } else if (kind == G4_ADDRESS) // address register
  {
    vISA_ASSERT(decl->getNumRows() == 1, ERROR_UNKNOWN);
    //
    // determine alignment
    // if the number of reg needed is more than 1, then we go ahead
    //
    unsigned regNeeded = numAllocUnit(decl->getNumElems(), decl->getElemType());
    if (findContiguousAddrFlag(FPR.availableAddrs, forbidden, subAlign,
                               regNeeded, builder.getNumAddrRegisters(), AS.startARFReg,
                               i)) {
      // subregoffset should consider the declare data type
      varBasis->setPhyReg(regPool.getAddrReg(),
                          i * G4_WSIZE / decl->getElemSize());
      return true;
    }
    return false;
  } else if (kind == G4_FLAG) // Flag register
  {
    vISA_ASSERT(decl->getNumRows() == 1, ERROR_UNKNOWN);
    //
    // determine alignment
    // if the number of reg needed is more than 1, then we go ahead
    //
    unsigned regNeeded = numAllocUnit(decl->getNumElems(), decl->getElemType());
    if (findContiguousAddrFlag(FPR.availableFlags, forbidden, subAlign,
                               regNeeded, builder.getNumFlagRegisters(),
                               AS.startFlagReg, i)) {
      // subregoffset should consider the declare data type
      varBasis->setPhyReg(regPool.getFlagAreg(i / 2), i & 1);
      return true;
    }
    return false;
  }
  else if (kind == G4_SCALAR) // scalar register
  {
    vISA_ASSERT(decl->getNumRows() == 1, ERROR_UNKNOWN);
    //
    // determine alignment
    // if the number of reg needed is more than 1, then we go ahead
    //
    unsigned regNeeded = numAllocUnit(decl->getNumElems(), decl->getElemType());
    if (findContiguousAddrFlag(
            FPR.availableScalars, forbidden, subAlign, regNeeded,
            builder.kernel.getSRFInWords(), AS.startScalarReg, i)) {
      // subregoffset should consider the declare data type
      varBasis->setPhyReg(regPool.getScalarReg(),
                          i * G4_WSIZE / decl->getElemSize());
      return true;
    }
    return false;
  }
  else // not handled yet
  {
    vISA_ASSERT(false, ERROR_UNKNOWN);
    return false;
  }
}

const BitSet *LiveRange::getForbidden() {
  return forbidden;
}

// Find the intersected RC for two reg classes, i.e., one that contains the
// forbidden registers for both RC.
static std::optional<forbiddenKind> intersectRegClass(forbiddenKind k1,
    forbiddenKind k2) {
  if (k1 == k2)
    return k1;
  if (isSubRegClass(k1, k2))
    return k1;
  if (isSubRegClass(k2, k1))
    return k2;
  // Special case: FBD_EOTLASTGRF = FBD_EOT & FBD_LASTGRF
  auto isEOTorLASTGRF = [](forbiddenKind kind) {
    return kind == forbiddenKind::FBD_LASTGRF || kind == forbiddenKind::FBD_EOT;
  };
  if (isEOTorLASTGRF(k1) && isEOTorLASTGRF(k2))
    return forbiddenKind::FBD_EOTLASTGRF;
  // TODO: Add more intersection RCs as the need arises.
  return std::nullopt;
}

// This function does not simply override the live range's RC to the new one;
// instead, it tries to find an intersected RC for the new and the existing RC,
// asserting if we can't find one among the pre-defined RCs.
// This both simplifies the caller (no need to manually perform intersection)
// and may help catch subtle bugs where different parts of code assign a live
// range to non-compatible RCs (e.g., caller-save and callee-save).
// If you want to force-override the existing RC, use resetForbidden() followed
// by setForbidden().
void LiveRange::setForbidden(forbiddenKind f) {
  // New RC should be a valid one.
  vASSERT(f != forbiddenKind::FBD_UNASSIGNED);
  auto newRC = intersectRegClass(f, forbiddenType);
  vISA_ASSERT(newRC, "can't find a valid intersection RC");
  if (newRC) {
    forbiddenType = *newRC;
    forbidden = gra.getForbiddenRegs(*newRC);
  } else {
    // In NDEBUG build, just overwrite the RC to keep with the old behavior.
    forbiddenType = f;
    forbidden = gra.getForbiddenRegs(f);
  }
}

void LiveRange::markForbidden(vISA::Mem_Manager &GCMem, int reg, int numReg) {
  BitSet *forbiddenBS = new (GCMem) BitSet(gra.getForbiddenVectorSize(G4_GRF), false);

  for (int i = 0; i < numReg; i++) {
    forbiddenBS->set(reg + i, true);
  }

  if (forbidden) {
    *forbiddenBS |= *forbidden;
  }

  forbidden = forbiddenBS;
}

int LiveRange::getNumForbidden() {
  return forbidden->count();
}

void getForbiddenGRFs(std::vector<unsigned int> &regNum, G4_Kernel &kernel,
                      unsigned stackCallRegSize, unsigned reserveSpillSize,
                      unsigned rerservedRegNum) {
  // Push forbidden register numbers to vector regNum
  //
  // r0 - Forbidden when platform is not 3d
  // rMax, rMax-1, rMax-2 - Forbidden in presence of stack call sites
  unsigned totalGRFNum = kernel.getNumRegTotal();

  // FIXME: We have way too many places that are reserving r0, they need to be
  // consolidated.
  if (kernel.getKernelType() != VISA_3D || !kernel.fg.builder->canWriteR0() ||
      reserveSpillSize > 0 || kernel.getOption(vISA_PreserveR0InR0)) {
    regNum.push_back(0);
  }

  if (kernel.fg.builder->mustReserveR1()) {
    regNum.push_back(1);
  }

  unsigned reservedRegSize = stackCallRegSize + reserveSpillSize;
  for (unsigned int i = 0; i < reservedRegSize; i++) {
    regNum.push_back(totalGRFNum - 1 - i);
  }

  unsigned largestNoneReservedReg = totalGRFNum - reservedRegSize - 1;
  if (totalGRFNum - reservedRegSize >= totalGRFNum - 16) {
    largestNoneReservedReg = totalGRFNum - 16 - 1;
  }

  if (totalGRFNum - reservedRegSize < rerservedRegNum) {
    vISA_ASSERT(false, "After reservation, there is not enough regiser!");
  }

  for (unsigned int i = 0; i < rerservedRegNum; i++) {
    regNum.push_back(largestNoneReservedReg - i);
  }
}

void getCallerSaveGRF(std::vector<unsigned int> &regNum, G4_Kernel *kernel) {
  unsigned int startCalleeSave = kernel->stackCall.calleeSaveStart();
  unsigned int endCalleeSave = startCalleeSave + kernel->stackCall.getNumCalleeSaveRegs();
  // r60-r124 are caller save regs for SKL
  for (unsigned int i = startCalleeSave; i < endCalleeSave; i++) {
    regNum.push_back(i);
  }
}

void getCalleeSaveGRF(std::vector<unsigned int> &regNum, G4_Kernel *kernel) {
  // r1-r59 are callee save regs for SKL
  unsigned int numCallerSaveGRFs = kernel->stackCall.getCallerSaveLastGRF() + 1;
  for (unsigned int i = 1; i < numCallerSaveGRFs; i++) {
    regNum.push_back(i);
  }
}

//
// print assigned reg info
//
void LiveRange::dump() const {
  G4_Declare *decl = var->getDeclare();
  this->emit(std::cout);
  std::cout << " : ";
  //
  // print alignment
  //
  std::cout << "\t";
  if (gra.getSubRegAlign(decl) == Any) {
    std::cout << "\t";
  } else {
    std::cout << gra.getSubRegAlign(decl) << "_words SubReg_Align";
  }
  //
  // dump number of registers that are needed
  //
  if (decl->getRegFile() == G4_ADDRESS) {
    std::cout << " + "
              << (IS_DTYPE(decl->getElemType()) ? 2 * decl->getNumElems()
                                                : decl->getNumElems())
              << " regs";
  } else {
    std::cout << "\t(" << decl->getNumRows() << "x" << decl->getNumElems()
              << "):" << TypeSymbol(decl->getElemType());
  }
}

PhyRegAllocationState::PhyRegAllocationState(
    GlobalRA &g, const LiveRangeVec &l, G4_RegFileKind r, unsigned int m,
    unsigned int bank1_s, unsigned int bank1_e, unsigned int bank2_s,
    unsigned int bank2_e, bool doBC, bool doBundleReduction)
    : gra(g),  rFile(r), maxGRFCanBeUsed(m), startARFReg(0), startFlagReg(0),
      startGRFReg(0), bank1_start(bank1_s), bank1_end(bank1_e),
      bank2_start(bank2_s), bank2_end(bank2_e), doBankConflict(doBC),
      doBundleConflict(doBundleReduction), lrs(l) {
  totalGRF = gra.kernel.getNumRegTotal();
  startScalarReg = 0;
}
