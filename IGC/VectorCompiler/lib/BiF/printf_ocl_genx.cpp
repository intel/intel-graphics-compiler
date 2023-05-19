/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/vector.h>
#include <opencl_def.h>

#include "printf_not_cm_common.h"
#include "vc/BiF/PrintfIface.h"

using namespace vc::bif::printf;
using namespace cm;

static constexpr int StringAnnotationSize = sizeof(BufferElementTy);
// String is transfered by its index, which is always 1 DWord.
static constexpr int StringArgSize = 1;

// Format string handling. Just writing format string index to buffer and
// promoting the pointer to buffer.
template <typename T>
vector<BufferElementTy, TransferDataSize>
printf_fmt_impl(vector<BufferElementTy, TransferDataSize> TransferData,
                T *FormatString) {
  if (TransferData[TransferDataLayout::ReturnValue])
    // Just skip.
    return TransferData;
  __global BufferElementTy *CurAddress = getCurAddress(TransferData);
  BufferElementTy Index = detail::printf_format_index(FormatString);
  CurAddress = writeElementToBuffer(CurAddress, Index);
  setCurAddress(TransferData, CurAddress);
  return TransferData;
}

// String argument requires a special treatment.
// It could've been covered in standard arg routine, but in this case pointer
// would have to pass through several bitcast, plus under condition. It would
// cause some problems as `@llvm.vc.internal.print.format.index` should get
// pointer directly from global constant. It would require several IR
// transformations to get rid of those bitcasts and conditions. Which can be
// avoided by this "specialization" for string argument.
template <typename T>
vector<BufferElementTy, TransferDataSize>
printf_arg_str_impl(vector<BufferElementTy, TransferDataSize> TransferData,
                    T *String) {
  if (TransferData[TransferDataLayout::ReturnValue])
    // Just skip.
    return TransferData;
  __global BufferElementTy *CurAddress = getCurAddress(TransferData);
  BufferElementTy Index = detail::printf_format_index(String);
  CurAddress = writeElementToBuffer(CurAddress, ArgCode::String);
  CurAddress = writeElementToBuffer(CurAddress, Index);
  setCurAddress(TransferData, CurAddress);
  return TransferData;
}

extern "C" cl_vector<BufferElementTy, TransferDataSize>
__vc_assert_init(cl_vector<int, ArgsInfoVector::Size> ArgsInfo) {
  return printf_init_impl<StringAnnotationSize, true>(ArgsInfo).cl_vector();
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
  return printf_arg_impl<StringArgSize>(TransferData, Kind, Arg).cl_vector();
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
