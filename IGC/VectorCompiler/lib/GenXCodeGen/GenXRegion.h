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
//
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

#ifndef GENXREGION_H
#define GENXREGION_H

#include "GenXAlignmentInfo.h"
#include "vc/GenXOpts/Utils/CMRegion.h"
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

// Region : description of an operand's region
class Region : public CMRegion {
public:
  static Region getWithOffset(Instruction *Inst, bool WantParentWith = false);
  // Default constructor: assume single element
  Region() : CMRegion() {}
  // Construct from a type.
  Region(Type *Ty, const DataLayout *DL = nullptr) : CMRegion(Ty, DL) {};
  // Construct from a value.
  Region(Value *V, const DataLayout *DL = nullptr) : CMRegion(V, DL) {};
  // Construct from a rd/wr region/element and its BaleInfo
  Region(Instruction *Inst, const BaleInfo &BI, bool WantParentWidth = false);
  // Construct from a bitmap of which elements to set (legal 1D region)
  Region(unsigned Bits, unsigned ElementBytes)
    : CMRegion(Bits, ElementBytes) {};
  // getLegalSize : get the max legal size of a region
  unsigned getLegalSize(unsigned Idx, bool Allow2D, unsigned InputNumElements,
                        const GenXSubtarget *ST, AlignmentInfo *AI = nullptr);
  unsigned getLegalSize(unsigned Idx, bool Allow2D, unsigned InputNumElements,
                        const GenXSubtarget *ST, Alignment Align);
};

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

Value *simplifyRegionInst(Instruction *Inst, const DataLayout *DL);
bool simplifyRegionInsts(Function *F, const DataLayout *DL);

bool cleanupLoads(Function *F);

bool IsLinearVectorConstantInts(Value* v, int64_t& start, int64_t& stride);

} // end namespace genx

} // end namespace llvm

#endif /* GENXREGION_H */
