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

  TypeSizeWrapper() = default;
  TypeSizeWrapper(DLTypeSize TS) : TS(TS){};
#if LLVM_VERSION_MAJOR >= 10
  TypeSizeWrapper(uint64_t TSIn) : TS{FixedDLSize(TSIn)} {};
#endif

  SzType inBytesCeil() const { return asIntegralCeil<ByteBits>(); }
  SzType inWordsCeil() const { return asIntegralCeil<WordBits>(); }
  SzType inDWordsCeil() const { return asIntegralCeil<DWordBits>(); }
  SzType inQWordsCeil() const { return asIntegralCeil<QWordBits>(); }
  SzType inOWordsCeil() const { return asIntegralCeil<OWordBits>(); }

  SzType inBits() const { return asIntegralStrict<1>(); }
  SzType inBytes() const { return asIntegralStrict<ByteBits>(); }
  SzType inWords() const { return asIntegralStrict<WordBits>(); }
  SzType inDWords() const { return asIntegralStrict<DWordBits>(); }
  SzType inQWords() const { return asIntegralStrict<QWordBits>(); }
  SzType inOWords() const { return asIntegralStrict<OWordBits>(); }

  friend bool operator==(const TypeSizeWrapper &LHS,
                         const TypeSizeWrapper &RHS) {
    return LHS.TS == RHS.TS;
  }
  friend bool operator<(const TypeSizeWrapper &LHS,
                        const TypeSizeWrapper &RHS) {
    return LHS.TS < RHS.TS;
  }
  friend bool operator!=(const TypeSizeWrapper &LHS,
                         const TypeSizeWrapper &RHS) {
    return !(LHS == RHS);
  }
  friend bool operator>=(const TypeSizeWrapper &LHS,
                         const TypeSizeWrapper &RHS) {
    return !(LHS < RHS);
  }
  friend bool operator<=(const TypeSizeWrapper &LHS,
                         const TypeSizeWrapper &RHS) {
    return (LHS < RHS) || (LHS == RHS);
  }
  friend bool operator>(const TypeSizeWrapper &LHS,
                        const TypeSizeWrapper &RHS) {
    return !(LHS <= RHS);
  }

private:
  template <unsigned UnitBitSize> SzType asIntegralStrict() const {
    return asIntegral<UnitBitSize, true>();
  }
  template <unsigned UnitBitSize> SzType asIntegralCeil() const {
    return asIntegral<UnitBitSize, false>();
  }

  template <unsigned UnitBitSize, bool Strict> SzType asIntegral() const {
#if LLVM_VERSION_MAJOR >= 10
    IGC_ASSERT(!TS.isScalable());
    uint64_t BitsAsUI = TS.getFixedSize();
#else
    uint64_t BitsAsUI = TS;
#endif
    IGC_ASSERT_MESSAGE(BitsAsUI <= std::numeric_limits<SzType>::max(),
                       "Type is too large to operate on");
    IGC_ASSERT_MESSAGE(BitsAsUI > 0, "Could not determine size of Type");
    if constexpr (Strict) {
      IGC_ASSERT_MESSAGE(BitsAsUI % UnitBitSize == 0,
          "Type size in bits cannot be represented in requested units exactly");
      return BitsAsUI / UnitBitSize;
    } else {
      return static_cast<SzType>(llvm::divideCeil(BitsAsUI, UnitBitSize));
    }
  }
  DLTypeSize TS = FixedDLSize(0);
};

inline const TypeSizeWrapper BoolSize{BoolBits};
inline const TypeSizeWrapper ByteSize{ByteBits};
inline const TypeSizeWrapper WordSize{WordBits};
inline const TypeSizeWrapper DWordSize{DWordBits};
inline const TypeSizeWrapper QWordSize{QWordBits};
inline const TypeSizeWrapper OWordSize{OWordBits};

// Utility function to get type size in diffrent units.
// TODO: make DataLyout as non-optional argument
TypeSizeWrapper getTypeSize(llvm::Type *Ty,
                            const llvm::DataLayout *DL = nullptr);

} // namespace vc

#endif /* end of include guard: VC_UTILS_GENX_TYPE_SIZE_H */
