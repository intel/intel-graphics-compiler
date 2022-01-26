/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_BIF_PRINTF_NOT_CM_COMMON_H
#define VC_BIF_PRINTF_NOT_CM_COMMON_H

#include <cm-cl/atomic.h>
#include <cm-cl/vector.h>
#include <opencl_def.h>

#include "vc/BiF/PrintfIface.h"

using namespace vc::bif::printf;
using namespace cm;

// Currently the max format string length supported by runtime.
static inline constexpr int MaxFormatStrSize = 16 * 1024;
// Number of vector elements for current address storage. Address is always
// stored as 64-bit value split into 2 parts (32-bit pointers are zext).
static inline constexpr int AddressVectorWidth = 2;

namespace TransferDataLayout {
enum Enum {
  // Indices:
  CurAddressLow,
  CurAddressHigh,
  ReturnValue,
};
} // namespace TransferDataLayout

using BufferElementTy = unsigned;
static inline constexpr int ArgHeaderSize = sizeof(BufferElementTy);

// StringAnnotationSize defines how much space in bytes is required to write
// a string to the prinf buffer. For ocl it is the size of a string index, for
// ze - the size of a string pointer.
template <int StringAnnotationSize>
inline int calcRequiredBufferSize(vector<int, ArgsInfoVector::Size> ArgsInfo) {
  int Num32BitArgs =
      ArgsInfo[ArgsInfoVector::NumTotal] - ArgsInfo[ArgsInfoVector::Num64Bit] -
      ArgsInfo[ArgsInfoVector::NumStr] - ArgsInfo[ArgsInfoVector::NumPtr];
  // Note that pointers are always passed as 64-bit values
  // (32-bit ones are zext).
  int Num64BitArgs =
      ArgsInfo[ArgsInfoVector::Num64Bit] + ArgsInfo[ArgsInfoVector::NumPtr];
  int BufferSize = StringAnnotationSize +
                   ArgsInfo[ArgsInfoVector::NumTotal] * ArgHeaderSize +
                   ArgsInfo[ArgsInfoVector::NumStr] * StringAnnotationSize +
                   Num32BitArgs * sizeof(int32_t) +
                   Num64BitArgs * sizeof(int64_t);
  return BufferSize;
}

// Return initial buffer offset in BufferElementTy elements (not in bytes).
static inline BufferElementTy
getInitialBufferOffset(__global BufferElementTy *BufferPtr,
                       BufferElementTy RequiredSize) {
#if __clang_major__ > 9
  int ByteOffset =
      atomic::execute<atomic::operation::add, memory_order_relaxed,
                      memory_scope_all_devices>(BufferPtr, RequiredSize);
#else  // __clang_major__ > 9
  // Helping clang-9 correctly deduce the argument type.
  int ByteOffset =
      atomic::execute<atomic::operation::add, memory_order_relaxed,
                      memory_scope_all_devices, __global BufferElementTy>(
          BufferPtr, RequiredSize);
#endif // __clang_major__ > 9
  return ByteOffset / sizeof(BufferElementTy);
}

template <typename T>
static vector<BufferElementTy, AddressVectorWidth> castPointerToVector(T *Ptr) {
  vector<uint64_t, 1> Tmp = reinterpret_cast<uintptr_t>(Ptr);
  return Tmp.format<BufferElementTy>();
}

// A helper function to properly set CurAddressLow and CurAddressHigh
// elements of \p TransferData vector by the provided \p Ptr.
static inline void
setCurAddress(vector<BufferElementTy, TransferDataSize> &TransferData,
              __global BufferElementTy *Ptr) {
  TransferData.select<AddressVectorWidth, 1>(
      TransferDataLayout::CurAddressLow) = castPointerToVector(Ptr);
}

// A helper function to properly extract current address from \p TransferData.
static inline __global BufferElementTy *
getCurAddress(vector<BufferElementTy, TransferDataSize> TransferData) {
  vector<BufferElementTy, AddressVectorWidth> Address =
      TransferData.select<AddressVectorWidth, 1>(
          TransferDataLayout::CurAddressLow);
  // Bit-casting to 64-bit int and then truncating if necessary.
  return reinterpret_cast<__global BufferElementTy *>(
      static_cast<uintptr_t>(Address.format<uint64_t>()));
}

static inline vector<BufferElementTy, TransferDataSize>
generateTransferData(__global BufferElementTy *InitPtr,
                     BufferElementTy ReturnValue) {
  vector<BufferElementTy, TransferDataSize> TransferData;
  setCurAddress(TransferData, InitPtr);
  TransferData[TransferDataLayout::ReturnValue] = ReturnValue;
  return TransferData;
}

// Printf initial routines. The function gets printf buffer and allocates
// space in it. It needs some info about args to allocate enough space.
template <int StringAnnotationSize>
vector<BufferElementTy, TransferDataSize>
printf_init_impl(vector<int, ArgsInfoVector::Size> ArgsInfo) {
  auto FmtStrSize = ArgsInfo[ArgsInfoVector::FormatStrSize];
  if (FmtStrSize > MaxFormatStrSize)
    return generateTransferData(/* BufferPtr */ nullptr, /* ReturnValue */ -1);
  auto BufferSize = calcRequiredBufferSize<StringAnnotationSize>(ArgsInfo);
#if __clang_major__ > 9
  auto *BufferPtr =
      static_cast<__global BufferElementTy *>(cm::detail::printf_buffer());
#else  // __clang_major__ > 9
  // clang-9 cannot handle this auto.
  __global BufferElementTy *BufferPtr =
      static_cast<__global BufferElementTy *>(cm::detail::printf_buffer());
#endif // __clang_major__ > 9
  auto Offset = getInitialBufferOffset(BufferPtr, BufferSize);
  return generateTransferData(BufferPtr + Offset, /* ReturnValue */ 0);
}

// Writes \p Data to printf buffer via \p CurAddress pointer.
// Returns promoted pointer.
static inline __global BufferElementTy *
writeElementToBuffer(__global BufferElementTy *CurAddress,
                     BufferElementTy Data) {
  *CurAddress = Data;
  return ++CurAddress;
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

// StringArgSize is in DWords.
template <int StringArgSize>
inline vector<BufferElementTy, ArgInfo::Size> getArgInfo(ArgKind::Enum Kind) {
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
    return RetInitT{ArgCode::String, StringArgSize};
  default:
    return RetInitT{ArgCode::Invalid, 0};
  }
}

// Single printf arg handling (those that are after format string).
// StringArgSize is in DWords.
template <int StringArgSize>
inline vector<BufferElementTy, TransferDataSize>
printf_arg_impl(vector<BufferElementTy, TransferDataSize> TransferData,
                ArgKind::Enum Kind,
                vector<BufferElementTy, ArgData::Size> Arg) {
  if (TransferData[TransferDataLayout::ReturnValue])
    // Just skip.
    return TransferData;
  vector<BufferElementTy, ArgInfo::Size> Info = getArgInfo<StringArgSize>(Kind);
  __global BufferElementTy *CurAddress = getCurAddress(TransferData);
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
