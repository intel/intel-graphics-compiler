/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "GenX.h"
#include "GenXUtil.h"
#include "vc/Utils/GenX/TypeSize.h"

#include "Probe/Assertion.h"
#include "RelocationInfo.h"

#include <llvm/ADT/APInt.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/MathExtras.h>

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace vc {

// Returns data represented as APInt. APInt bit width is equal to \p C bit size.
// Relocations are returned in vector with offsets relative to constant
// beginning.
std::pair<llvm::APInt, std::vector<vISA::ZERelocEntry>>
encodeGlobalValueOrConstantExpression(const llvm::Constant &C,
                                      const llvm::DataLayout &DL);

// Transforms the provided relocations - adds additional \p Offset to their
// original offset. Original relocations are copied or moved depending on
// iterator kind.
template <typename InputIter, typename OutputIter>
void shiftRelocations(InputIter First, InputIter Last, OutputIter Out,
                      std::size_t Offset) {
  std::transform(First, Last, Out, [Offset](vISA::ZERelocEntry Reloc) {
    Reloc.r_offset += Offset;
    return Reloc;
  });
}

// This class encodes byte representation of the provided constant.
// The data is written byte by byte (char by char).
// The storage pointed by DataOutIter must have sufficient space to preserve
// encoded constant. The storage pointed by RelocOutIter must have sufficient
// space to preserve all the required relocations.
template <typename DataOutIter, typename RelocOutIter> class ConstantEncoder {
  const llvm::DataLayout &DL;
  // The number of already written bytes. It is updated by emit functions.
  std::size_t NumWrittenBytes = 0;
  // Do not write directly to it, use emit functions.
  DataOutIter DataIt;
  RelocOutIter RelocIt;

public:
  ConstantEncoder(const llvm::DataLayout &DLIn, DataOutIter DataItIn,
                  RelocOutIter RelocItIn)
      : DL{DLIn}, DataIt{DataItIn}, RelocIt{RelocItIn} {}

  std::size_t encode(const llvm::Constant &Const) {
    return switchConstants(Const);
  }

private:
  // encodeLeafConstImpl is a set of overloaded functions that encode
  // different types of constants.
  // Mind that having large overloading set over derived types is error-prone.
  // Thus no type of encodeLeafConstImpl argument is derived from a type of
  // a different encodeLeafConstImpl argument. Most of them are leaf types
  // in herritage tree.
  std::size_t encodeLeafConstImpl(const llvm::ConstantFP &ConstFP) {
    llvm::APInt API = ConstFP.getValueAPF().bitcastToAPInt();
    IGC_ASSERT_MESSAGE(API.getBitWidth() == llvm::genx::QWordBits ||
                           API.getBitWidth() == llvm::genx::DWordBits ||
                           API.getBitWidth() == llvm::genx::WordBits,
                       "only doulbe, float and half are supported");
    auto Size = API.getBitWidth() / llvm::genx::ByteBits;
    emitIntValue(API.getZExtValue(), Size);
    return Size;
  }

  std::size_t encodeLeafConstImpl(const llvm::ConstantStruct &ConstStruct) {
    std::size_t TotalSize = 0;
    auto *Layout = DL.getStructLayout(ConstStruct.getType());
    for (unsigned i = 0, e = ConstStruct.getNumOperands(); i != e; ++i) {
      llvm::Constant *ConstElem = ConstStruct.getOperand(i);
      auto WrittenBytes = switchConstants(*ConstElem);
      // Potential padding.
      auto PaddedElemSize = llvm::genx::getStructElementPaddedSize(
          i, ConstStruct.getNumOperands(), *Layout);
      TotalSize += PaddedElemSize;
      emitZeros(PaddedElemSize - WrittenBytes);
    }
    return TotalSize;
  }

  std::size_t
  encodeLeafConstImpl(const llvm::ConstantDataSequential &ConstDataSeq) {
    IGC_ASSERT_MESSAGE(DL.isBigEndian() == llvm::sys::IsBigEndianHost,
                       "this fast raw method works only when target and host "
                       "endianness are the same");
    llvm::StringRef Data = ConstDataSeq.getRawDataValues();
    emitData(Data.begin(), Data.end());
    return Data.size();
  }

  std::size_t encodeLeafConstImpl(const llvm::ConstantInt &ConstInt) {
    IGC_ASSERT_MESSAGE(DL.getTypeAllocSize(ConstInt.getType()) <=
                           llvm::genx::QWordBytes,
                       "max i64 type is yet supported");
    auto Size = DL.getTypeAllocSize(ConstInt.getType());
    emitIntValue(ConstInt.getZExtValue(), Size);
    return Size;
  }

  std::size_t encodeLeafConstImpl(const llvm::ConstantArray &ConstArray) {
    return encodeHomogenAggregate(ConstArray,
                                  ConstArray.getType()->getElementType());
  }

  std::size_t encodeLeafConstImpl(const llvm::ConstantVector &ConstVector) {
    return encodeHomogenAggregate(ConstVector,
                                  ConstVector.getType()->getElementType());
  }

  // To not forget eventually consider this case.
  std::size_t encodeLeafConstImpl(const llvm::ConstantTokenNone &ConstTN) {
    IGC_ASSERT_MESSAGE(0, "constant token none is yet unsupported");
    return 0;
  }

  // A helper function to switch all constant types and call the proper
  // encode implementation.
  std::size_t switchConstants(const llvm::Constant &Const) {
    if (llvm::isa<llvm::ConstantData>(Const))
      return switchConstantsInner(llvm::cast<llvm::ConstantData>(Const));
    if (llvm::isa<llvm::ConstantExpr>(Const) ||
        llvm::isa<llvm::GlobalValue>(Const))
      return handleGVOrCE(Const);
    return switchConstantsInner(llvm::cast<llvm::ConstantAggregate>(Const));
  }

  std::size_t switchConstantsInner(const llvm::ConstantData &CData) {
    if (llvm::isa<llvm::ConstantDataSequential>(CData))
      return encodeLeafConstImpl(
          llvm::cast<llvm::ConstantDataSequential>(CData));
    if (llvm::isa<llvm::ConstantFP>(CData))
      return encodeLeafConstImpl(llvm::cast<llvm::ConstantFP>(CData));
    if (llvm::isa<llvm::ConstantInt>(CData))
      return encodeLeafConstImpl(llvm::cast<llvm::ConstantInt>(CData));
    if (llvm::isa<llvm::ConstantTokenNone>(CData))
      return encodeLeafConstImpl(llvm::cast<llvm::ConstantTokenNone>(CData));
    return encodeZeroedConstant(CData);
  }

  std::size_t handleGVOrCE(const llvm::Constant &Const) {
    auto [Data, Relocations] = encodeGlobalValueOrConstantExpression(Const, DL);
    // FIXME: add more restrict TypeSizeWrapper method that asserts that size
    // is exactly in bytes.
    auto Size = vc::TypeSizeWrapper{Data.getBitWidth()}.inBytes();
    auto *DataBegin = reinterpret_cast<const char *>(Data.getRawData());
    shiftRelocations(std::make_move_iterator(Relocations.begin()),
                     std::make_move_iterator(Relocations.end()), RelocIt,
                     NumWrittenBytes);
    emitData(DataBegin, DataBegin + Size);
    return Size;
  }

  std::size_t switchConstantsInner(const llvm::ConstantAggregate &CAggr) {
    if (llvm::isa<llvm::ConstantArray>(CAggr))
      return encodeLeafConstImpl(llvm::cast<llvm::ConstantArray>(CAggr));
    if (llvm::isa<llvm::ConstantStruct>(CAggr))
      return encodeLeafConstImpl(llvm::cast<llvm::ConstantStruct>(CAggr));
    return encodeLeafConstImpl(llvm::cast<llvm::ConstantVector>(CAggr));
  }

  // \p ElemTy is the considered aggregate element type.
  std::size_t encodeHomogenAggregate(const llvm::ConstantAggregate &ConstArray,
                                     llvm::Type *ElemTy) {
    IGC_ASSERT_MESSAGE(ElemTy, "wrong argument");
    auto ElemPaddedSize = DL.getTypeAllocSize(ElemTy);
    std::size_t TotalSize = 0;
    for (unsigned i = 0, e = ConstArray.getNumOperands(); i != e; ++i) {
      llvm::Constant *ConstElem = ConstArray.getOperand(i);
      auto WrittenBytes = switchConstants(*ConstElem);
      // potential padding
      IGC_ASSERT_MESSAGE(ElemPaddedSize >= WrittenBytes,
                         "alloc size can only be bigger due to padding");
      TotalSize += ElemPaddedSize;
      emitZeros(ElemPaddedSize - WrittenBytes);
    }
    return TotalSize;
  }

  std::size_t encodeZeroedConstant(const llvm::ConstantData &ZeroedConst) {
    IGC_ASSERT_MESSAGE(llvm::isa<llvm::ConstantAggregateZero>(ZeroedConst) ||
                           llvm::isa<llvm::UndefValue>(ZeroedConst) ||
                           llvm::isa<llvm::ConstantPointerNull>(ZeroedConst),
                       "wrong argument type");
    auto Size = DL.getTypeStoreSize(ZeroedConst.getType());
    emitZeros(Size);
    return Size;
  }

  void emitIntValue(uint64_t Value, unsigned Size) {
    IGC_ASSERT_MESSAGE(1 <= Size && Size <= sizeof(decltype(Value)),
                       "Invalid size");
    IGC_ASSERT_MESSAGE((llvm::isUIntN(llvm::genx::ByteBits * Size, Value) ||
                        llvm::isIntN(llvm::genx::ByteBits * Size, Value)),
                       "Invalid size");
    IGC_ASSERT_MESSAGE(DL.isLittleEndian(),
                       "only little-endian targets are supported");
    for (unsigned i = 0; i != Size; ++i) {
      auto Shifted = Value >> (i * llvm::genx::ByteBits);
      auto Masked = Shifted & llvm::maskTrailingOnes<decltype(Shifted)>(
                                  llvm::genx::ByteBits);
      *DataIt++ = static_cast<char>(Masked);
    }
    NumWrittenBytes += Size;
  }

  void emitZeros(std::size_t Size) {
    std::fill_n(DataIt, Size, 0);
    NumWrittenBytes += Size;
  }

  template <typename ForwardIter>
  void emitData(ForwardIter First, ForwardIter Last) {
    std::copy(First, Last, DataIt);
    NumWrittenBytes += std::distance(First, Last);
  }
};

template <typename DataOutIter, typename RelocOutIter>
ConstantEncoder(const llvm::DataLayout &, DataOutIter, RelocOutIter)
    ->ConstantEncoder<DataOutIter, RelocOutIter>;

template <typename DataOutIterT, typename RelocOutIterT>
std::size_t encodeConstant(const llvm::Constant &Const,
                           const llvm::DataLayout &DL, DataOutIterT DataOutIt,
                           RelocOutIterT RelocOutIt) {
  return ConstantEncoder<DataOutIterT, RelocOutIterT>{DL, DataOutIt, RelocOutIt}
      .encode(Const);
}

} // namespace vc
