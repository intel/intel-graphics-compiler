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
/// CMRegion : region information
/// -------------------------------
/// 
/// An object of class CMRegion describes the region parameters of a Gen region.
/// It is a transient object, in that a pass constructs it as needed and then
/// forgets it. It does not persist between passes, as the region parameters are
/// fully described by the arguments to the rdregion and wrregion intrinsics.
///
/// The region parameters in a CMRegion are:
///
/// * ElementBytes : number of bytes per element
/// * ElementTy : Type of element
/// * NumElements : total number of elements in the region (number of rows is
///   thus NumElements/Width)
/// * VStride : vertical stride in elements
/// * Width : row width in elements
/// * Stride : horizontal stride in elements
/// * Offset : constant part of offset
/// * Indirect : variable index (nullptr for direct region, scalar value for
///   single indirect, vector value for multi indirect)
/// * IndirectIdx : start index in vector indirect. This is always 0 when
///   constructing a CMRegion, but can be set to a non-zero value before
///   calling a method to create a new rdregion/wrregion intrinsic
/// * IndirectAddrOffset : offset from the address value where region
///   origin starts
/// * Mask : mask (predicate) for wrregion, nullptr if none
/// * ParentWidth : the parent width value (a statement that no row crosses a
///   boundary of a multiple of this number of elements)
///
/// There are the following constructors:
///
/// * Construct from a Type or Value, setting the GenXRegion to a region that
///   covers the whole value.
/// * Construct from a rdregion/wrregion intrinsic, setting the GenXRegion to the
///   region described by the intrinsic.
/// * Construct from a bitmap of which elements need to be in the region. This
///   is used from GenXConstants when constructing a splat region when loading
///   a constant in multiple stages.
/// 
/// CMRegion is not used to represent the region parameters in predicate regions,
/// since they are much simpler. But GenXRegion does contain static methods to create
/// rdpredregion etc intrinsics given the predicate region parameters.
/// 
//===----------------------------------------------------------------------===//

#ifndef CMREGION_H
#define CMREGION_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"

namespace llvm {

class Constant;
class DataLayout;
class Value;
class Function;
class Module;
class Type;
class Instruction;
class raw_ostream;
class Twine;
class DebugLoc;
class TargetLibraryInfo;

// CMRegion : description of an operand's region
class CMRegion {
public:
  unsigned ElementBytes;
  Type *ElementTy;
  unsigned NumElements;
  int VStride;
  unsigned Width;
  int Stride;
  int Offset;
  Value *Indirect;
  unsigned IndirectIdx; // start index in vector Indirect
  unsigned IndirectAddrOffset;
  Value *Mask;          // 0 else mask for wrregion
  unsigned ParentWidth; // 0 else parent width
  // Default constructor: assume single element
  CMRegion()
      : ElementBytes(0), ElementTy(0), NumElements(1), VStride(1), Width(1),
        Stride(1), Offset(0), Indirect(0), IndirectIdx(0),
        IndirectAddrOffset(0), Mask(0), ParentWidth(0) {}
  // Construct from a type.
  CMRegion(Type *Ty, const DataLayout *DL = nullptr);
  // Construct from a value.
  CMRegion(Value *V, const DataLayout *DL = nullptr);
  // Construct from a rd/wr region/element
  CMRegion(Instruction *Inst, bool WantParentWidth = false);
  // Construct from a bitmap of which elements to set (legal 1D region)
  CMRegion(unsigned Bits, unsigned ElementBytes);
  // Create rdregion intrinsic from this Region
  // Returns a scalar if the Region has one element and AllowScalar is true.
  // Otherwise returns a vector.
  Instruction *createRdRegion(Value *Input, const Twine &Name,
                              Instruction *InsertBefore, const DebugLoc &DL,
                              bool AllowScalar = false);
  // Modify Region object for a subregion
  void getSubregion(unsigned StartIdx, unsigned Size);
  // Create wrregion intrinsic from this Region
  Value *createWrRegion(Value *OldVal, Value *Input, const Twine &Name,
                        Instruction *InsertBefore, const DebugLoc &DL);
  // Create wrconstregion intrinsic from this Region
  Value *createWrConstRegion(Value *OldVal, Value *Input, const Twine &Name,
                             Instruction *InsertBefore, const DebugLoc &DL);
  // Create rdpredregion from given start index and size
  static Instruction *createRdPredRegion(Value *Input, unsigned Index,
                                         unsigned Size, const Twine &Name,
                                         Instruction *InsertBefore,
                                         const DebugLoc &DL);
  static Value *createRdPredRegionOrConst(Value *Input, unsigned Index,
                                          unsigned Size, const Twine &Name,
                                          Instruction *InsertBefore,
                                          const DebugLoc &DL);
  // Create wrpredregion from given start index
  static Instruction *createWrPredRegion(Value *OldVal, Value *Input,
                                         unsigned Index, const Twine &Name,
                                         Instruction *InsertBefore,
                                         const DebugLoc &DL);
  // Create wrpredpredregion from given start index
  static Instruction *createWrPredPredRegion(Value *OldVal, Value *Input,
                                             unsigned Index, Value *Pred,
                                             const Twine &Name,
                                             Instruction *InsertBefore,
                                             const DebugLoc &DL);
  // Set the called function in an intrinsic call
  static void setRegionCalledFunc(Instruction *Inst);
  // Compare two regions to see if they have the same region parameters other
  // than start offset (not allowing element type to be different).
  bool isStrictlySimilar(const CMRegion &R2) const {
    return VStride == R2.VStride && Width == R2.Width && Stride == R2.Stride &&
           Mask == R2.Mask;
  }
  // Compare two regions to see if they have the same region parameters other
  // than start offset (also allowing element type to be different).
  bool isSimilar(const CMRegion &R2) const;
  // Compare two regions to see if they have the same region parameters (also
  // allowing element type to be different).
  bool operator==(const CMRegion &R2) const {
    return isSimilar(R2) && Offset == R2.Offset && Indirect == R2.Indirect
        && IndirectIdx == R2.IndirectIdx;
  }
  bool operator!=(const CMRegion &R2) const { return !(*this == R2); }
  // Compare two regions to see if they overlaps each other.
  bool overlap(const CMRegion &R2) const;
  // Test whether a region is scalar
  bool isScalar() const {
    return !Stride && (Width == NumElements || !VStride);
  }
  // Test whether a region is 2D
  bool is2D() const { return !isScalar() && Width != NumElements; }
  // Test whether a region is contiguous.
  bool isContiguous() const;
  // Test whether a region covers exactly the whole of the given type, allowing
  // for the element type being different.
  bool isWhole(Type *Ty) const;
  // Test whether the region has a whole number of rows. (append() can result
  // in a region with an incomplete final row, which is normally not allowed.)
  bool isWholeNumRows() const { return !(NumElements % Width); }
  // Evaluate rdregion with constant input.
  Constant *evaluateConstantRdRegion(Constant *Input, bool AllowScalar);
  // evaluateConstantWrRegion : evaluate wrregion with constant inputs
  Constant *evaluateConstantWrRegion(Constant *OldVal, Constant *NewVal);
  // append : append region AR to this region
  bool append(CMRegion AR);
  // changeElementType : change the element type of the region
  bool changeElementType(Type *NewElementType);
  // Debug dump/print
  void dump() const;
  void print(raw_ostream &OS) const;
  // Check whether the region is multi indirect. Returns true if Indirect has
  // VectorType (a sign of multi indirection)
  bool isMultiIndirect() const {
    return Indirect && isa<VectorType>(Indirect->getType());
  }
  // Get bit mask in which ones values represent bytes which
  // were accessed by this region
  BitVector getAccessBitMap(int MinTrackingOffset = 0) const;
  // Length of single row in bytes
  unsigned getRowLength() const {
    return Stride ? (Width * Stride * ElementBytes) : ElementBytes;
  }
  // Length of whole region in bytes
  unsigned getLength() const {
    return VStride * ((NumElements / Width) - 1) * ElementBytes +
                getRowLength();
  }

protected:
  // Create wrregion or wrconstregion intrinsic from this Region
  Value *createWrCommonRegion(GenXIntrinsic::ID, Value *OldVal, Value *Input,
                              const Twine &Name, Instruction *InsertBefore,
                              const DebugLoc &DL);
  // Get the function declaration for a region intrinsic
  static Function *getGenXRegionDeclaration(Module *M, GenXIntrinsic::ID IID, Type *RetTy,
                                        ArrayRef<Value *> Args);
  // Get (or create instruction for) the start index of a region.
  Value *getStartIdx(const Twine &Name, Instruction *InsertBefore, const DebugLoc &DL);
};

inline raw_ostream &operator<<(raw_ostream &OS, const CMRegion &R) {
  R.print(OS);
  return OS;
}

} // end namespace llvm 

#endif /* CMREGION_H */
