/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/vector.h>
#include <opencl_def.h>

#include "printf_not_cm_common.h"
#include "vc/BiF/PrintfIface.h"

using namespace vc::bif::printf;
using namespace cm;

static constexpr int StringAnnotationSize = sizeof(uintptr_t);
// StringAnnotationSize but in DWords.
static constexpr int StringDWordSize = StringAnnotationSize / 4;

// Format string handling. Just writing format string pointer to buffer and
// promoting the pointer to buffer.
template <typename T>
vector<BufferElementTy, TransferDataSize>
printf_fmt_impl(vector<BufferElementTy, TransferDataSize> TransferData,
                T *FormatString) {
  if (TransferData[TransferDataLayout::ReturnValue])
    // Just skip.
    return TransferData;
  __global BufferElementTy *CurAddress = getCurAddress(TransferData);
  auto StrAddress = castPointerToVector(FormatString);
  for (int Idx = 0; Idx != StringDWordSize; ++Idx)
    CurAddress = writeElementToBuffer(CurAddress, StrAddress[Idx]);
  setCurAddress(TransferData, CurAddress);
  return TransferData;
}

// String must be handled separately in order ocl printf to work.
// It can be treated as any other argument in zebin case.
template <typename T>
vector<BufferElementTy, TransferDataSize>
printf_arg_str_impl(vector<BufferElementTy, TransferDataSize> TransferData,
                    T *String) {
  return printf_arg_impl<StringDWordSize>(TransferData, ArgKind::String,
                                          castPointerToVector(String));
}

extern "C" cl_vector<BufferElementTy, TransferDataSize>
__vc_printf_init(cl_vector<int, ArgsInfoVector::Size> ArgsInfo) {
  return printf_init_impl<StringAnnotationSize>(ArgsInfo).cl_vector();
}

extern "C" cl_vector<BufferElementTy, TransferDataSize>
__vc_printf_fmt(cl_vector<BufferElementTy, TransferDataSize> TransferData,
                __constant char *FormatString) {
  return printf_fmt_impl(TransferData, FormatString).cl_vector();
}

extern "C" cl_vector<BufferElementTy, TransferDataSize> __vc_printf_fmt_global(
    cl_vector<BufferElementTy, TransferDataSize> TransferData,
    __global char *FormatString) {
  return printf_fmt_impl(TransferData, FormatString).cl_vector();
}

// legacy VC IR has no address spaces, so every pointer is "private".
extern "C" cl_vector<BufferElementTy, TransferDataSize> __vc_printf_fmt_legacy(
    cl_vector<BufferElementTy, TransferDataSize> TransferData,
    __private char *FormatString) {
  return printf_fmt_impl(TransferData, FormatString).cl_vector();
}

extern "C" cl_vector<BufferElementTy, TransferDataSize>
__vc_printf_arg(cl_vector<BufferElementTy, TransferDataSize> TransferData,
                ArgKind::Enum Kind,
                cl_vector<BufferElementTy, ArgData::Size> Arg) {
  return printf_arg_impl<StringDWordSize>(TransferData, Kind, Arg).cl_vector();
}

extern "C" cl_vector<BufferElementTy, TransferDataSize>
__vc_printf_arg_str(cl_vector<BufferElementTy, TransferDataSize> TransferData,
                    __constant char *String) {
  return printf_arg_str_impl(TransferData, String).cl_vector();
}

extern "C" cl_vector<BufferElementTy, TransferDataSize>
__vc_printf_arg_str_global(
    cl_vector<BufferElementTy, TransferDataSize> TransferData,
    __global char *String) {
  return printf_arg_str_impl(TransferData, String).cl_vector();
}

// legacy VC IR has no address spaces, so every pointer is "private".
extern "C" cl_vector<BufferElementTy, TransferDataSize>
__vc_printf_arg_str_legacy(
    cl_vector<BufferElementTy, TransferDataSize> TransferData,
    __private char *String) {
  return printf_arg_str_impl(TransferData, String).cl_vector();
}

extern "C" int
__vc_printf_ret(cl_vector<BufferElementTy, TransferDataSize> TransferData) {
  return printf_ret_impl(TransferData);
}
