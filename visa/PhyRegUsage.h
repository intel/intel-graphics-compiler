/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __PHYREGUSAGE_H__
#define __PHYREGUSAGE_H__

#include "Assertions.h"
#include "BuildIR.h"
#include "G4_IR.hpp"
#include "G4_Opcode.h"

enum ColorHeuristic { FIRST_FIT, ROUND_ROBIN };

// forward declares
namespace vISA {
class LiveRange;
class GlobalRA;
} // namespace vISA

using LiveRangeVec = std::vector<vISA::LiveRange*>;
namespace vISA {
// Allocation state shared by all PhyRegUsage objects (and their associated live
// range). This needs to be shared because the previous live range's allocation
// may affect the current live range's allocation (e.g., if we are doing
// round-robin assignment, the current live range's start location will be right
// after previous one's).
// FIXME: This design of a global state used by PhyRegUssage object is really
// bad, we should pass the starting register to each PhyRegUsage's ctor.
class PhyRegAllocationState {
  friend class PhyRegUsage;

  GlobalRA &gra;
  G4_RegFileKind rFile;
  unsigned int maxGRFCanBeUsed;
  unsigned int startARFReg;
  unsigned int startFlagReg;
  unsigned int startGRFReg;
  // FIXME: I don't understand why this needs to be shared, shouldn't the bank
  // assignment come from each live range?
  unsigned int bank1_start;
  unsigned int bank1_end;
  unsigned int bank2_start;
  unsigned int bank2_end;
  bool doBankConflict;
  bool doBundleConflict;
  // FIXME: Why do we need both totalGRF and maxGRFCanBeUsed?
  unsigned int totalGRF;
  const LiveRangeVec& lrs;

public:
  PhyRegAllocationState() = delete;
  PhyRegAllocationState(GlobalRA &g, const LiveRangeVec& l,
                        G4_RegFileKind r,
                        unsigned int m, unsigned int bank1_s,
                        unsigned int bank1_e, unsigned int bank2_s,
                        unsigned int bank2_e, bool doBC,
                        bool doBundleReduction);

  void setStartGRF(unsigned startGRF) { startGRFReg = startGRF; }
};

// Class representing available physical registers (GRF, address, flag, etc.)
// that may be assigned to a variable. This is a separate class because it is
// shared by PhyRegUsage objects (which is per live-range) to avoid having to
// allocate/deallocate them for each live range.
class FreePhyRegs {
  friend class PhyRegUsage;

  G4_Kernel &K;
  // Existing code uses C-style bool array, and we choose to keep it (rather
  // than say vector<bool> or bitset), as these objects are frequently accessed
  // and we want to avoid bit-manipulation overhead. The max number of physical
  // registers is also small, so we won't waste much space compared to
  // bit-arrays.
  bool *availableGregs;
  // 16-bit (32-bit for 64 byte GRF) mask marking the free words within one
  // GRF.
  std::vector<uint32_t> availableSubRegs;
  bool *availableAddrs;
  bool *availableFlags;
  std::vector<uint8_t> weakEdgeUsage;

public:
  FreePhyRegs() = delete;
  FreePhyRegs(G4_Kernel &kernel) : K(kernel) {
    availableGregs = new bool[K.getNumRegTotal()];
    std::fill_n(availableGregs, K.getNumRegTotal(), true);
    availableSubRegs.resize(kernel.getNumRegTotal(), UINT_MAX);
    availableAddrs = new bool[getNumAddrRegisters()];
    std::fill_n(availableAddrs, getNumAddrRegisters(), true);
    availableFlags = new bool[K.fg.builder->getNumFlagRegisters()];
    std::fill_n(availableFlags, K.fg.builder->getNumFlagRegisters(), true);
    // Note that unlike other fields this is initialized to false.
    weakEdgeUsage.resize(K.getNumRegTotal(), 0);
  }

  ~FreePhyRegs() {
    delete availableGregs;
    delete availableAddrs;
    delete availableFlags;
  }

  void reset() {
    std::fill_n(availableGregs, K.getNumRegTotal(), true);
    std::fill(availableSubRegs.begin(), availableSubRegs.end(), UINT_MAX);
    std::fill_n(availableAddrs, getNumAddrRegisters(), true);
    std::fill_n(availableFlags, K.fg.builder->getNumFlagRegisters(), true);
    std::fill(weakEdgeUsage.begin(), weakEdgeUsage.end(), 0);
  }
};

//
// track which registers are currently in use (cannot be assigned to other
// variables) For sub reg allocation, the granularity is UW/W (2 bytes). Doing
// so, we only need to handle even and odd alignment.
//
class PhyRegUsage {
  GlobalRA &gra;
  const LiveRangeVec& lrs;
  unsigned maxGRFCanBeUsed;
  ColorHeuristic colorHeuristic; // perform register assignment in
                                 // first-fit/round-robin for GRFs
  G4_RegFileKind regFile;
  // Reference to global free phyreg arrays.
  FreePhyRegs &FPR;
  // Reference to global allocation state.
  PhyRegAllocationState &AS;

  unsigned totalGRFNum;

  bool honorBankBias; // whether we honor the bank bias assigned by the bank
                      // conflict avoidance heuristic
  bool avoidBundleConflict; // whether avoid bundle conflict or not
  bool overlapTest;   // set to true only when current dcl has compatible ranges
                      // marked by augmentation

  struct PhyReg {
    int reg;
    int subreg; // in unit of words (0-15)
  };            // return type for findGRFSubReg

  PhyReg findGRFSubReg(const BitSet *forbidden, bool callerSaveBias,
                       bool callerSaverBias, BankAlign align,
                       G4_SubReg_Align subAlign, unsigned nwords);

  void findGRFSubRegFromRegs(int startReg, int endReg, int step, PhyReg *phyReg,
                             G4_SubReg_Align subAlign, unsigned nwords,
                             const BitSet *forbidden,
                             bool fromPartialOccupiedReg);

  PhyReg findGRFSubRegFromBanks(G4_Declare *dcl, const BitSet *forbidden,
                                bool oneGRFBankDivision);

  void freeGRFSubReg(unsigned regNum, unsigned regOff, unsigned nwords,
                     G4_Type ty);
  void freeContiguous(bool availRegs[], unsigned start, unsigned numReg,
                      unsigned maxRegs);
  bool canGRFSubRegAlloc(G4_Declare *decl);
  bool findContiguousNoWrapGRF(bool availRegs[], const BitSet *forbidden,
                               unsigned short occupiedBundles, BankAlign align,
                               unsigned numRegNeeded, unsigned startPos,
                               unsigned endPos, unsigned &idx);

  bool findContiguousNoWrapAddrFlag(bool availRegs[], const BitSet *forbidden,
                                    G4_SubReg_Align subAlign,
                                    unsigned numRegNeeded, unsigned startPos,
                                    unsigned endPos, unsigned &idx);

  bool findFreeRegs(bool availRegs[], const BitSet *forbidden, BankAlign align,
                    unsigned numRegNeeded, unsigned startRegNum,
                    unsigned endRegNum, unsigned &idx, bool gotoSecondBank,
                    bool oneGRFBankDivision);

public:
  IR_Builder &builder;

  PhyRegPool &regPool; // all Physical Reg Operands

  PhyRegUsage(PhyRegAllocationState &, FreePhyRegs &);

  bool isOverlapValid(unsigned int, unsigned int);

  void setWeakEdgeUse(unsigned int reg, uint8_t index) {
    // Consider V1 is allocated to r10, r11, r12, r13.
    // Then following will be set eventually to model
    // compatible ranges:
    //  weakEdgeUsage[10] = 1;
    //  weakEdgeUsage[11] = 2;
    //  weakEdgeUsage[12] = 3;
    //  weakEdgeUsage[13] = 4;
    // This means some other compatible range cannot start
    // at r7, r8, r9, r11, r12, r13. Another compatible range
    // can either have no overlap at all with this range (strong
    // edge), or it can start at r10 to have full
    // overlap (weak edge).
    FPR.weakEdgeUsage[reg] = index;
  }

  uint8_t getWeakEdgeUse(unsigned int reg) const {
    return FPR.weakEdgeUsage[reg];
  }

  void runOverlapTest(bool t) { overlapTest = t; }

  ~PhyRegUsage() {}

  bool assignRegs(bool isSIMD16, LiveRange *var, const BitSet *forbidden,
                  BankAlign align, G4_SubReg_Align subAlign,
                  ColorHeuristic colorHeuristic, float spillCost, bool hintSet);

  bool assignGRFRegsFromBanks(LiveRange *varBasis, BankAlign align,
                              const BitSet *forbidden, ColorHeuristic heuristic,
                              bool oneGRFBankDivision);

  void markBusyForDclSplit(G4_RegFileKind kind, unsigned regNum,
                           unsigned regOff, unsigned nunits, unsigned numRows);

  void markBusyGRF(unsigned regNum, unsigned regOff, unsigned nunits,
                   unsigned numRows, bool isPreDefinedVar) {
    vISA_ASSERT(numRows > 0 && nunits > 0, ERROR_INTERNAL_ARGUMENT);

    vISA_ASSERT((regNum + numRows <= maxGRFCanBeUsed) || isPreDefinedVar,
                 ERROR_UNKNOWN);

    //
    // sub reg allocation (allocation unit is word)
    //
    if (numRows == 1 && regOff + nunits < builder.numEltPerGRF<Type_UW>()) {
      FPR.availableGregs[regNum] = false;
      auto subregMask = getSubregBitMask(regOff, nunits);
      FPR.availableSubRegs[regNum] &= ~subregMask;
    } else // allocate whole registers
    {
      for (unsigned i = 0; i < numRows; i++) {
        FPR.availableGregs[regNum + i] = false;
        if (builder.getGRFSize() == 64)
          FPR.availableSubRegs[regNum + i] = 0;
        else
          FPR.availableSubRegs[regNum + i] = 0xffff0000;
      }
    }
  }

  void markBusyAddress(unsigned regNum, unsigned regOff, unsigned nunits,
                       unsigned numRows) {
    vISA_ASSERT(regNum == 0 && regOff + nunits <= getNumAddrRegisters(),
                 ERROR_UNKNOWN);
    for (unsigned i = regOff; i < regOff + nunits; i++)
      FPR.availableAddrs[i] = false;
  }

  void markBusyFlag(unsigned regNum, unsigned regOff, unsigned nunits,
                    unsigned numRows) {
    for (unsigned i = regOff; i < regOff + nunits; i++)
      FPR.availableFlags[i] = false;
  }
  static unsigned numAllocUnit(unsigned nelems, G4_Type ty) {
    //
    // we allocate sub reg in 2-byte granularity
    //
    unsigned nbytes = nelems * TypeSize(ty);
    return nbytes / G4_WSIZE + nbytes % G4_WSIZE;
  }

  // translate offset to allocUnit
  static unsigned offsetAllocUnit(unsigned nelems, G4_Type ty) {

    unsigned nbytes = nelems * TypeSize(ty);
    // RA allocate register in unit of G4_WSIZE bytes
    // pre-assigned register may start from nbytes%G4_WSIZE != 0, i.e, within an
    // allocUnit
    return nbytes / G4_WSIZE;
  }

  void updateRegUsage(LiveRange *lr);

  uint32_t getSubregBitMask(uint32_t start, uint32_t num) const {
    vISA_ASSERT(num > 0 && start + num <= builder.numEltPerGRF<Type_UW>(),
                 "illegal number of words");
    uint32_t mask = ((1 << num) - 1) << start;

    return (uint32_t)mask;
  }

  void emit(std::ostream &output) {
    output << "available GRFs: ";
    for (unsigned int i = 0; i < totalGRFNum; i++) {
      if (FPR.availableGregs[i]) {
        output << i << " ";
      }
    }
    output << "\n";
  }

private:
  void freeRegs(LiveRange *var);

  bool findContiguousAddrFlag(bool availRegs[], const BitSet *forbidden,
                              G4_SubReg_Align subAlign, unsigned numRegNeeded,
                              unsigned maxRegs,
                              unsigned &startReg, // inout
                              unsigned &idx,      // output
                              bool isCalleeSaveBias = false,
                              bool isEOTSrc = false);

  bool findContiguousGRFFromBanks(G4_Declare *dcl, bool availRegs[],
                                  const BitSet *forbidden, BankAlign align,
                                  unsigned &idx, bool oneGRFBankDivision);

  unsigned short getOccupiedBundle(const G4_Declare *dcl) const;

  // find contiguous free words in a registers
  int findContiguousWords(uint32_t words, G4_SubReg_Align alignment,
                          int numWord) const;
  bool findContiguousGRF(bool availRegs[], const BitSet *forbidden,
                         unsigned occupiedBundles, BankAlign align,
                         unsigned numRegNeeded, unsigned maxRegs,
                         unsigned &startPos, unsigned &idx,
                         bool isCalleeSaveBias, bool isEOTSrc);
};
} // namespace vISA
#endif // __PHYREGUSAGE_H__
