/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
/// * Construct from a rdregion/wrregion intrinsic, setting the GenXRegion to
/// the
///   region described by the intrinsic.
/// * Construct from a bitmap of which elements need to be in the region. This
///   is used from GenXConstants when constructing a splat region when loading
///   a constant in multiple stages.
///
/// CMRegion is not used to represent the region parameters in predicate
/// regions, since they are much simpler. But GenXRegion does contain static
/// methods to create rdpredregion etc intrinsics given the predicate region
/// parameters.
///
//===----------------------------------------------------------------------===//

#ifndef VC_UTILS_GENX_REGION_H
#define VC_UTILS_GENX_REGION_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"

namespace llvm {
class Constant;
class DataLayout;
class DebugLoc;
class Function;
class Instruction;
class Module;
class Twine;
class Type;
class Value;
class VectorType;
class raw_ostream;
} // namespace llvm

namespace vc {
namespace WrPredRegionOperand {
enum { OldValue, NewValue, Offset };
} // namespace WrPredRegionOperand

// CMRegion : description of an operand's region
class CMRegion {
  using Constant = llvm::Constant;
  using DataLayout = llvm::DataLayout;
  using DebugLoc = llvm::DebugLoc;
  using Function = llvm::Function;
  using Instruction = llvm::Instruction;
  using Module = llvm::Module;
  using Twine = llvm::Twine;
  using Type = llvm::Type;
  using Value = llvm::Value;
  using VectorType = llvm::VectorType;
  using raw_ostream = llvm::raw_ostream;

public:
  unsigned ElementBytes;
  Type *ElementTy;
  unsigned NumElements;
  int VStride;
  unsigned Width;
  int Stride;
  // NOTE: with the current design region may have both non-zero \p Offset and
  //       set \p Indirect, which means indirect access + constant offset, which
  //       is a form of indirect access.
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
  CMRegion(const Value *V, const DataLayout *DL = nullptr);
  // Construct from a rd/wr region/element
  CMRegion(const Instruction *Inst, bool WantParentWidth = false);
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
  Instruction *createWrRegion(Value *OldVal, Value *Input, const Twine &Name,
                              Instruction *InsertBefore, const DebugLoc &DL);
  // Create wrconstregion intrinsic from this Region
  Instruction *createWrConstRegion(Value *OldVal, Value *Input,
                                   const Twine &Name, Instruction *InsertBefore,
                                   const DebugLoc &DL);
  // Create rdpredregion from given start index and size
  static Instruction *createRdPredRegion(Value *Input, unsigned Index,
                                         unsigned Size, const Twine &Name,
                                         Instruction *InsertBefore,
                                         const DebugLoc &DL);
  static Value *createRdPredRegionOrConst(Value *Input, unsigned Index,
                                          unsigned Size, const Twine &Name,
                                          Instruction *InsertBefore,
                                          const DebugLoc &DL);
  // Create rdregion representing vector splat
  static Value *createRdVectorSplat(const DataLayout &DL, unsigned NumElements,
                                    Value *Input, const Twine &Name,
                                    Instruction *InsertBefore,
                                    const DebugLoc &DbgLoc);
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
    return isSimilar(R2) && Offset == R2.Offset && Indirect == R2.Indirect &&
           IndirectIdx == R2.IndirectIdx;
  }
  bool operator!=(const CMRegion &R2) const { return !(*this == R2); }
  // Compare two regions to see if they overlaps each other.
  bool overlap(const CMRegion &R2) const;
  // Test whether a region is scalar
  bool isScalar() const {
    return !Stride && (Width == NumElements || !VStride);
  }
  // Checks whether region <vstride;width;stride> can really be represented as
  // 1D region <stride'>.
  bool is1D() const;
  // For 1D region (can be represented as <stride>) return the stride.
  // The behavior is undefined for not 1D regions.
  int get1DStride() const;
  // For destination region (1D region) returns its stride. Unlike get1DStride
  // cannot return 0 stride.
  // The behavior is undefined for not 1D regions.
  int getDstStride() const;
  // Test whether a region is 2D
  bool is2D() const { return !isScalar() && Width != NumElements; }
  // Test whether a region is contiguous.
  bool isContiguous() const;
  // Test whether a region covers exactly the whole of the given type, allowing
  // for the element type being different.
  bool isWhole(Type *Ty, const DataLayout *DL = nullptr) const;
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
  bool changeElementType(Type *NewElementType, const DataLayout *DL);
  // Debug dump/print
  void dump() const;
  void print(raw_ostream &OS) const;
  // Check whether the region is multi indirect. Returns true if Indirect has
  // VectorType (a sign of multi indirection)
  bool isMultiIndirect() const {
    return Indirect && llvm::isa<VectorType>(Indirect->getType());
  }
  // Get indices in vector that represent accessed elements for this region
  llvm::SmallVector<unsigned, 8> getAccessIndices() const;
  // Get bit mask in which ones values represent bytes which
  // were accessed by this region
  llvm::SmallBitVector getAccessBitMap(int MinTrackingOffset = 0) const;
  // Length of single row in bytes
  unsigned getRowLength() const {
    return Stride ? (Width * Stride * ElementBytes) : ElementBytes;
  }
  // Length of whole region in bytes
  unsigned getLength() const {
    return VStride * ((NumElements / Width) - 1) * ElementBytes +
           getRowLength();
  }

  // Returns the region offset in muber of elements.
  // This method cannot be called for (multi-)indirect regions.
  unsigned getOffsetInElements() const;

  // Returns selected region type. Corresonds to rdregion return value type or
  // wrregion new value operand type.
  // Set \p UseDegenerateVectorType to produce <1 x Ty> instead of Ty.
  Type *getRegionType(bool UseDegenerateVectorType = false) const;

  // Sets \p ElementTy field with \p Ty and sets \p ElementByte accordingly.
  // Data layout \p DL must be provided for pointer types.
  void setElementTy(Type *Ty, DataLayout *DL = nullptr);

protected:
  // Create wrregion or wrconstregion intrinsic from this Region
  Instruction *createWrCommonRegion(llvm::GenXIntrinsic::ID IID, Value *OldVal,
                                    Value *Input, const Twine &Name,
                                    Instruction *InsertBefore,
                                    const DebugLoc &DL);
  // Get the function declaration for a region intrinsic
  static Function *getGenXRegionDeclaration(Module *M,
                                            llvm::GenXIntrinsic::ID IID,
                                            Type *RetTy,
                                            llvm::ArrayRef<Value *> Args);
  // Get (or create instruction for) the start index of a region.
  Value *getStartIdx(const Twine &Name, Instruction *InsertBefore,
                     const DebugLoc &DL);
};

/* Note: Region is a more specialized class for constructing Regions,
   the primary difference is that Region class requires only Value interface
   and is not aware about Instruction stuff.
*/
class Region : public CMRegion {
  using DataLayout = llvm::DataLayout;
  using Type = llvm::Type;
  using Value = llvm::Value;

public:
  // Default constructor: assume single element
  Region() : CMRegion() {}
  // Construct from a type.
  Region(Type *Ty, const DataLayout *DL = nullptr) : CMRegion(Ty, DL){};
  // Construct from a value.
  Region(const Value *V, const DataLayout *DL = nullptr) : CMRegion(V, DL){};
  // Construct from a bitmap of which elements to set (legal 1D region)
  Region(unsigned Bits, unsigned ElementBytes) : CMRegion(Bits, ElementBytes){};
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const CMRegion &R) {
  R.print(OS);
  return OS;
}
} // namespace vc

#endif // VC_UTILS_GENX_REGION_H
