/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/math.h>
#include <cm-cl/vector.h>

using namespace cm;

namespace {
CM_NODEBUG CM_INLINE vector<int16_t, 32 * 16>
reduce_first_step(vector<int16_t, 32 * 32> Src) {
  constexpr int NumRows = 16;
  constexpr int16_t AbsMask = 0x7FFF;

  vector<int16_t, 32 * NumRows> Res;

#pragma unroll
  for (int I = 0; I < 32; I += 2) {
    auto Row0Offset = I * 32;
    auto Row1Offset = (I + 1) * 32;

    vector<int16_t, 32> Row0 = Src.template select<32, 1>(Row0Offset);
    vector<int16_t, 32> Row1 = Src.template select<32, 1>(Row1Offset);

    Row0 = Row0 & AbsMask;
    Row1 = Row1 & AbsMask;

    auto ResOffset = I / 2 * 32;
    auto ResRow = Res.template select<32, 1>(ResOffset);
    auto ResEven = ResRow.template select<16, 2>(0);
    auto ResOdd = ResRow.template select<16, 2>(1);

    vector<int16_t, 16> Row0Even = Row0.template select<16, 2>(0);
    vector<int16_t, 16> Row0Odd = Row0.template select<16, 2>(1);
    ResEven = math::maximum(Row0Odd, Row0Even); // argument order is important

    vector<int16_t, 16> Row1Even = Row1.template select<16, 2>(0);
    vector<int16_t, 16> Row1Odd = Row1.template select<16, 2>(1);
    ResOdd = math::maximum(Row1Even, Row1Odd); // argument order is important
  }

  return Res;
}

template <int Height>
CM_NODEBUG CM_INLINE vector<int16_t, 32 * Height / 2>
reduce_next_step(vector<int16_t, 32 * Height> Src) {
  constexpr int NumRows = Height / 2;

  vector<int16_t, 32 * NumRows> Res;

#pragma unroll
  for (int I = 0; I < Height; I += 2) {
    auto Row0Offset = I * 32;
    auto Row1Offset = (I + 1) * 32;

    vector<int16_t, 32> Row0 = Src.template select<32, 1>(Row0Offset);
    vector<int16_t, 32> Row1 = Src.template select<32, 1>(Row1Offset);

    auto ResOffset = I / 2 * 32;
    auto ResRow = Res.template select<32, 1>(ResOffset);
    auto ResEven = ResRow.template select<16, 2>(0);
    auto ResOdd = ResRow.template select<16, 2>(1);

    vector<int16_t, 16> Low0 = Row0.template select<16, 1>(0);
    vector<int16_t, 16> High0 = Row0.template select<16, 1>(16);
    ResEven = math::maximum(High0, Low0); // argument order is important

    vector<int16_t, 16> Low1 = Row1.template select<16, 1>(0);
    vector<int16_t, 16> High1 = Row1.template select<16, 1>(16);
    ResOdd = math::maximum(High1, Low1); // argument order is important
  }

  return Res;
}

CM_NODEBUG CM_INLINE vector<int16_t, 32> reduce(vector<int16_t, 32 * 32> Src) {
  auto Step1 = reduce_first_step(Src);
  auto Step2 = reduce_next_step<16>(Step1);
  auto Step3 = reduce_next_step<8>(Step2);
  auto Step4 = reduce_next_step<4>(Step3);
  auto Step5 = reduce_next_step<2>(Step4);
  return Step5;
}

CM_NODEBUG CM_INLINE vector<int16_t, 32> linearize(vector<int16_t, 32> Src) {
  cl_vector<uint32_t, 16> CIndices = {0, 8, 4, 12, 2, 10, 6, 14,
                                      1, 9, 5, 13, 3, 11, 7, 15};
  vector<uint32_t, 16> Indices(CIndices);
  Indices *= 0x11;

  vector<uint32_t, 16> SrcI = Src.template format<uint32_t>();
  vector<uint8_t, 64> IndexBytes = Indices.template format<uint8_t>();

  auto ShuffleI = math::upconvert_4bit_lut<0>(SrcI, IndexBytes);
  auto Shuffle = ShuffleI.template format<int16_t>();
  auto ShuffleEven = Shuffle.template select<16, 2>(0);
  auto ShuffleOdd = Shuffle.template select<16, 2>(1);

  vector<int16_t, 32> Res;
  auto ResLow = Res.template select<16, 1>(0);
  auto ResHigh = Res.template select<16, 1>(16);

  ResLow = ShuffleEven;
  ResHigh = ShuffleOdd;

  return Res;
}
} // namespace

CM_NODEBUG CM_INLINE extern "C" cl_vector<int16_t, 32>
__vc_builtin_mxfp_reduce_32x32(cl_vector<int16_t, 32 * 32> Src) {
  return reduce(Src).cl_vector();
}

CM_NODEBUG CM_INLINE extern "C" cl_vector<int16_t, 32>
__vc_builtin_mxfp_linearize(cl_vector<int16_t, 32> Src) {
  return linearize(Src).cl_vector();
}
