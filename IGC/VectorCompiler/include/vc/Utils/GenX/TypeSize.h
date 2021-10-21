/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_TYPE_SIZE_H
#define VC_UTILS_GENX_TYPE_SIZE_H

#include <llvm/IR/Type.h>
#include <llvm/Support/MathExtras.h>

#include <llvmWrapper/IR/DerivedTypes.h>

#include "Probe/Assertion.h"

namespace llvm {
class DataLayout;
}

namespace vc {

constexpr unsigned BoolBits = 1;
constexpr unsigned ByteBits = 8;
constexpr unsigned WordBits = 16;
constexpr unsigned DWordBits = 32;
constexpr unsigned QWordBits = 64;
constexpr unsigned OWordBits = 128;

// TODO: probably TypeSizeWrapper should go to the general IGC wrappers
class TypeSizeWrapper {
public:
  using SzType = unsigned;

#if LLVM_VERSION_MAJOR >= 10
  using DLTypeSize = llvm::TypeSize;
  static DLTypeSize InvalidDLSize() {
    return DLTypeSize::Fixed(0);
  }
  static DLTypeSize FixedDLSize(uint64_t SZ) {
    return DLTypeSize::Fixed(SZ);
  }
#else
  using DLTypeSize = uint64_t;
  static DLTypeSize InvalidDLSize() {
    return 0;
  }
  static DLTypeSize FixedDLSize(uint64_t SZ) {
    return SZ;
  }
#endif

  TypeSizeWrapper(DLTypeSize TS) : TS(TS){};
#if LLVM_VERSION_MAJOR >= 10
  TypeSizeWrapper(uint64_t TSIn) : TS{FixedDLSize(TSIn)} {};
#endif

  SzType inBits() const { return asIntegral<1>(); }
  SzType inBytes() const { return asIntegral<ByteBits>(); }
  SzType inWords() const { return asIntegral<WordBits>(); }
  SzType inDWords() const { return asIntegral<DWordBits>(); }
  SzType inQWords() const { return asIntegral<QWordBits>(); }
  SzType inOWords() const { return asIntegral<OWordBits>(); }

  bool operator==(const TypeSizeWrapper &Other) const { return TS == Other.TS; }

private:
  template <unsigned UnitBitSize> SzType asIntegral() const {
#if LLVM_VERSION_MAJOR >= 10
    IGC_ASSERT(!TS.isScalable());
    uint64_t BitsAsUI = TS.getFixedSize();
#else
    uint64_t BitsAsUI = TS;
#endif
    IGC_ASSERT_MESSAGE(BitsAsUI <= std::numeric_limits<SzType>::max(),
                       "Type is too large to operate on");
    IGC_ASSERT_MESSAGE(BitsAsUI > 0, "Could not determine size of Type");
    return static_cast<SzType>(llvm::divideCeil(BitsAsUI, UnitBitSize));
  }
  DLTypeSize TS;
};

// Utility function to get type size in diffrent units.
// TODO: make DataLyout as non-optional argument
TypeSizeWrapper getTypeSize(llvm::Type *Ty,
                            const llvm::DataLayout *DL = nullptr);

} // namespace vc

#endif /* end of include guard: VC_UTILS_GENX_TYPE_SIZE_H */
