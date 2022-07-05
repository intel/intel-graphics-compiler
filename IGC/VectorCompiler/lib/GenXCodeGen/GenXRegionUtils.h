/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/// GenXRegion : region information
/// -------------------------------
///
/// Refer to the comments in the base class CMRegion defined in
/// llvm/Transform/Scalar.
///
/// Function added for the GenXRegion
///
/// * Construct from a rdregion/wrregion intrinsic, setting the GenXRegion
///   to the region described by the intrinsic. This constructor also takes the
///   BaleInfo as an argument, allowing a variable index that is a baled in
///   constant add to be considered as a separate variable index and constant
///   offset.
///
/// GenXLegalization uses GenXRegion to determine whether a region is legal,
/// and split it up if necessary. First it constructs a GenXRegion, then it
/// has a loop to split it into legal regions. Each loop iteration calls:
///
/// * the getLegalSize method (see below) to determine the split size; then
/// * getSubregion to modify the GenXRegion for the split size; then
/// * one of the methods to create a new rdregion or wrregion intrinsic.
///
/// GenXRegion::getLegalSize
/// ^^^^^^^^^^^^^^^^^^^^^^^^
///
/// The ``getLegalSize`` method is used by GenXLegalization and some other
/// passes to determine whether a region is legal, and if not how small
/// a split is required to make it legal.
///
/// It takes the GenXSubtarget as an argument, because it needs to know
/// architecture-specific details, currently just whether a single GRF
/// crossing is allowed in an indirect region.
///
/// It also takes either an AlignmentInfo object, or the actual alignment
/// of the indirect index (if any). Knowing the alignment of the indirect
/// index can help allow a larger legal region, and avoid needing to split
/// into simd1.
///
//===----------------------------------------------------------------------===//

#ifndef LIB_GENXCODEGEN_GENXREGIONUTILS_H
#define LIB_GENXCODEGEN_GENXREGIONUTILS_H

#include "GenXAlignmentInfo.h"
#include "vc/Utils/GenX/Region.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallBitVector.h"

namespace llvm {
    class Constant;
    class DataLayout;
    class Value;
    class Function;
    class GenXBaling;
    class GenXSubtarget;
    class Module;
    class Type;
    class Instruction;
    class raw_ostream;
    class Twine;
    class DebugLoc;
    class TargetLibraryInfo;

namespace genx {

struct BaleInfo;

using Region = vc::Region;

Region makeRegionWithOffset(const Instruction *Inst,
                            bool WantParentWidth = false);

Region makeRegionFromBaleInfo(const Instruction *Inst, const BaleInfo &BI,
                              bool WantParentWidth = false);

// getLegalSize : get the max legal size of a region
unsigned getLegalRegionSizeForTarget(const GenXSubtarget &ST, const Region &R,
                                     unsigned Idx, bool Allow2D,
                                     unsigned InputNumElements,
                                     AlignmentInfo *AI = nullptr);
unsigned getLegalRegionSizeForTarget(const GenXSubtarget &ST, const Region &R,
                                     unsigned Idx, bool Allow2D,
                                     unsigned InputNumElements,
                                     Alignment Align);

// RdWrRegionSequence : a sequence of rdregion-wrregion pairs probably
// created by legalization or coalescing, conforming to the following
// rules:
//
// 1. It is a sequence of wrregions, each one (other than the last)
//    having the next one's "old value" input as its only use.
//
// 2. Each wrregion's "new value" input is a single-use rdregion.
//
// 3. All the rdregions have the same "old value" input.
//
// 4. If the rdregions have a variable index, the index is the same for each
//    one, other than the constant offset from a baled in genx.add.addr.
//
// 5. The rdregion regions are such that they can be combined to give the
//    region parameters of the original unsplit rdregion. Those rdregion
//    parameters are stored in the RdR field.
//
// 6. If the wrregions have a variable index, the index is the same for each
//    one, other than the constant offset from a baled in genx.add.addr.
//
// 7. The wrregion regions are such that they can be combined to give the
//    region parameters of the original unsplit wrregion. Those wrregion
//    parameters are stored in the WrR field.
//
// Alternatively, a RdWrRegionSequence can represent a sequence of wrregion
// instructions with undef "old value" input to the first one and constant
// "new value" input to each one, forming a legalized constant load.
//
class RdWrRegionSequence {
  Instruction *WaitingFor = nullptr;
public:
  Value *Input = nullptr;
  Value *OldVal = nullptr;
  Instruction *StartWr = nullptr;
  Instruction *EndWr = nullptr;
  Region RdR;
  Region WrR;
  // Default constructor
  RdWrRegionSequence() : Input(nullptr), EndWr(nullptr) {}
  // isNull : true if the RdWrRegionSequence has not been initialized
  bool isNull() const { return !EndWr && !Input; }
  // Scan for sequence from the start wrregion instruction.
  // Returns false if not even a single rdregion-wrregion pair found.
  bool buildFromStartWr(Instruction *Wr, GenXBaling *Baling);
  // Scan for sequence from any wrregion instruction in the sequence.
  // Returns false if not even a single rdregion-wrregion pair found.
  bool buildFromWr(Instruction *Wr, GenXBaling *Baling);
  // Scan for sequence from any rdregion instruction in the sequence.
  // Returns false if not even a single rdregion-wrregion pair found.
  bool buildFromRd(Instruction *Rd, GenXBaling *Baling);
  // Get number of rdregion-wrregion pairs in the sequence
  unsigned size() const;
  // Check whether the sequence is the only use of its input
  bool isOnlyUseOfInput() const;
  // Get the index of the legalized rdregion
  Value *getRdIndex() const;
  // Get the index of the legalized wrregion
  Value *getWrIndex() const;
  // Get some use of Input in the sequence
  Use *getInputUse() const;
  // Debug dump/print
  void dump() const;
  void print(raw_ostream &OS) const;
};

inline raw_ostream &operator<<(raw_ostream &OS, const RdWrRegionSequence &RWS) {
  RWS.print(OS);
  return OS;
}

Value *simplifyRegionInst(Instruction *Inst, const DataLayout *DL = nullptr,
                          const GenXSubtarget *ST = nullptr);
bool simplifyRegionInsts(Function *F, const DataLayout *DL = nullptr,
                         const GenXSubtarget *ST = nullptr);

bool cleanupLoads(Function *F);

bool IsLinearVectorConstantInts(Value* v, int64_t& start, int64_t& stride);

} // end namespace genx

} // end namespace llvm

#endif // LIB_GENXCODEGEN_GENXREGIONUTILS_H
