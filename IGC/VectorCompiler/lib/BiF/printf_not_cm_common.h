/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_BIF_PRINTF_NOT_CM_COMMON_H
#define VC_BIF_PRINTF_NOT_CM_COMMON_H

#include <cm-cl/svm.h>
#include <cm-cl/vector.h>
#include <opencl_def>

#include "vc/BiF/PrintfIface.h"

using namespace vc::bif::printf;
using namespace cm;

// Currently the max format string length supported by runtime.
static inline constexpr int MaxFormatStrSize = 16 * 1024;

namespace TransferDataLayout {
enum Enum {
  // Indices:
  CurAddressLow,
  CurAddressHigh,
  ReturnValue,
  // Number of vector elements for current address storage. Address is always
  // stored as 64-bit value split into 2 parts (32-bit pointers are zext).
  AddressSize = 2
};
} // namespace TransferDataLayout

using BufferElementTy = unsigned;
static inline constexpr int ArgHeaderSize = sizeof(BufferElementTy);
static inline constexpr int FormatStringAnnotationSize =
    sizeof(BufferElementTy);

static inline int
calcRequiredBufferSize(vector<int, ArgsInfoVector::Size> ArgsInfo) {
  int Num32BitArgs = ArgsInfo[ArgsInfoVector::NumTotal] -
                     ArgsInfo[ArgsInfoVector::Num64Bit] -
                     ArgsInfo[ArgsInfoVector::NumPtr];
  // Note that pointers is always passed as 64-bit values
  // (32-bit ones are zext).
  int Num64BitArgs =
      ArgsInfo[ArgsInfoVector::Num64Bit] + ArgsInfo[ArgsInfoVector::NumPtr];
  int BufferSize = FormatStringAnnotationSize +
                   ArgsInfo[ArgsInfoVector::NumTotal] * ArgHeaderSize +
                   Num32BitArgs * sizeof(int32_t) +
                   Num64BitArgs * sizeof(int64_t);
  return BufferSize;
}

static inline BufferElementTy getInitialBufferOffset(uintptr_t BufferPtr,
                                                     int RequiredSize) {
  constexpr int MagicNumber = 8;
  constexpr cl_vector<uint64_t, MagicNumber> AddrOffset{0,  4,  8,  12,
                                                        16, 20, 24, 28};
  vector<BufferElementTy, MagicNumber> Result;
  vector<BufferElementTy, MagicNumber> Size = 0;
  Size[0] = RequiredSize;
  vector<uint64_t, MagicNumber> Offsets(AddrOffset);
  vector<uintptr_t, MagicNumber> Addr = BufferPtr + Offsets;

  Result = svm::atomic<svm::operation::add>(Addr, Size);
  return Result[0];
}

// A helper function to properly set CurAddressLow and CurAddressHigh
// elements of \p TransferData vector by the provided \p Ptr.
static inline void
setCurAddress(vector<BufferElementTy, TransferDataSize> &TransferData,
              uintptr_t Ptr) {
  vector<uint64_t, 1> Tmp = Ptr;
  TransferData.select<TransferDataLayout::AddressSize, 1>(
      TransferDataLayout::CurAddressLow) = Tmp.format<BufferElementTy>();
}

// A helper function to properly extract current address from \p TransferData.
static inline uintptr_t
getCurAddress(vector<BufferElementTy, TransferDataSize> TransferData) {
  vector<BufferElementTy, TransferDataLayout::AddressSize> Address =
      TransferData.select<TransferDataLayout::AddressSize, 1>(
          TransferDataLayout::CurAddressLow);
  // Bit-casting to 64-bit int and then truncating if necessary.
  return Address.format<uint64_t>();
}

static inline vector<BufferElementTy, TransferDataSize>
generateTransferData(uintptr_t InitPtr, BufferElementTy ReturnValue) {
  vector<BufferElementTy, TransferDataSize> TransferData;
  setCurAddress(TransferData, InitPtr);
  TransferData[TransferDataLayout::ReturnValue] = ReturnValue;
  return TransferData;
}

// Printf initial routines. The function gets printf buffer and allocates
// space in it. It needs some info about args to allocate enough space.
static inline vector<BufferElementTy, TransferDataSize>
printf_init_impl(vector<int, ArgsInfoVector::Size> ArgsInfo) {
  auto FmtStrSize = ArgsInfo[ArgsInfoVector::FormatStrSize];
  if (FmtStrSize > MaxFormatStrSize)
    return generateTransferData(/* BufferPtr */ 0, /* ReturnValue */ -1);
  auto BufferSize = calcRequiredBufferSize(ArgsInfo);
  auto BufferPtr = reinterpret_cast<uintptr_t>(cm::detail::printf_buffer());
  auto Offset = getInitialBufferOffset(BufferPtr, BufferSize);
  return generateTransferData(BufferPtr + Offset, /* ReturnValue */ 0);
}

// Writes \p Data to printf buffer via \p CurAddress pointer.
// Returns promoted pointer.
static inline uintptr_t writeElementToBuffer(uintptr_t CurAddress,
                                             BufferElementTy Data) {
  vector<uintptr_t, 1> CurAddressVec = CurAddress;
  vector<BufferElementTy, 1> DataVec = Data;
  svm::scatter(CurAddressVec, DataVec);
  return CurAddress + sizeof(Data);
}

// ArgCode is written into printf buffer before every argument.
namespace ArgCode {
enum Enum {
  Invalid,
  Byte,
  Short,
  Int,
  Float,
  String,
  Long,
  Pointer,
  Double,
  VectorByte,
  VectorShort,
  VectorInt,
  VectorLong,
  VectorFloat,
  VectorDouble,
  Size
};
} // namespace ArgCode

namespace ArgInfo {
enum Enum { Code, NumDWords, Size };
} // namespace ArgInfo

static inline vector<BufferElementTy, ArgInfo::Size>
getArgInfo(ArgKind::Enum Kind) {
  using RetInitT = cl_vector<BufferElementTy, ArgInfo::Size>;
  switch (Kind) {
  case ArgKind::Char:
  case ArgKind::Short:
  case ArgKind::Int:
    return RetInitT{ArgCode::Int, 1};
  case ArgKind::Long:
    return RetInitT{ArgCode::Long, 2};
  case ArgKind::Float:
    return RetInitT{ArgCode::Float, 1};
  case ArgKind::Double:
    return RetInitT{ArgCode::Double, 2};
  case ArgKind::Pointer:
    return RetInitT{ArgCode::Pointer, 2};
  case ArgKind::String:
    return RetInitT{ArgCode::String, 1};
  default:
    return RetInitT{ArgCode::Invalid, 0};
  }
}

// Single printf arg handling (those that are after format string).
static inline vector<BufferElementTy, TransferDataSize>
printf_arg_impl(vector<BufferElementTy, TransferDataSize> TransferData,
                ArgKind::Enum Kind,
                vector<BufferElementTy, ArgData::Size> Arg) {
  if (TransferData[TransferDataLayout::ReturnValue])
    // Just skip.
    return TransferData;
  vector<BufferElementTy, ArgInfo::Size> Info = getArgInfo(Kind);
  uintptr_t CurAddress = getCurAddress(TransferData);
  CurAddress = writeElementToBuffer(CurAddress, Info[ArgInfo::Code]);
  for (int Idx = 0; Idx != Info[ArgInfo::NumDWords]; ++Idx)
    CurAddress = writeElementToBuffer(CurAddress, Arg[Idx]);
  setCurAddress(TransferData, CurAddress);
  return TransferData;
}

// Getting printf return value here.
static inline int
printf_ret_impl(vector<BufferElementTy, TransferDataSize> TransferData) {
  return TransferData[TransferDataLayout::ReturnValue];
}

#endif // VC_BIF_PRINTF_NOT_CM_COMMON_H
