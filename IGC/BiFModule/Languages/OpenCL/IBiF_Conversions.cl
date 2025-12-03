/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===-  IBiF_Conversions.cl - OpenCL explicit conversion functions   -===//
//
// This file defines builtin versions of OpenCL explicit conversion
// functions.
//
//===----------------------------------------------------------------------===//

#include "IBiF_Conversions_long_emulation.cl"
#include "spirv.h"

#define RT_POS 0
#define RT_NEG 1
#define RT_Z   2
#define RT_NE  3

uchar OVERLOADABLE convert_uchar(uchar _T) {
    return __spirv_UConvert_Ruchar( _T );
}

uchar OVERLOADABLE convert_uchar(ushort _T) {
    return __spirv_UConvert_Ruchar( _T );
}

uchar OVERLOADABLE convert_uchar(uint _T) {
    return __spirv_UConvert_Ruchar( _T );
}

uchar OVERLOADABLE convert_uchar(ulong _T) {
    return __spirv_UConvert_Ruchar( _T );
}

uchar OVERLOADABLE convert_uchar(char _T) {
    return __spirv_SConvert_Rchar( _T );
}

uchar OVERLOADABLE convert_uchar(short _T) {
    return __spirv_SConvert_Rchar( _T );
}

uchar OVERLOADABLE convert_uchar(int _T) {
    return __spirv_SConvert_Rchar( _T );
}

uchar OVERLOADABLE convert_uchar(long _T) {
    return __spirv_SConvert_Rchar( _T );
}

uchar OVERLOADABLE convert_uchar(float _T) {
    return __spirv_ConvertFToU_Ruchar( _T );
}

uchar OVERLOADABLE convert_uchar_rte(uchar _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rte(ushort _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rte(uint _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rte(ulong _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rte(char _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rte(short _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rte(int _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rte(long _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rte(float _T) {
    return __spirv_ConvertFToU_Ruchar_rte( _T );
}

uchar OVERLOADABLE convert_uchar_rtp(uchar _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtp(ushort _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtp(uint _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtp(ulong _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtp(char _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtp(short _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtp(int _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtp(long _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtp(float _T) {
    return __spirv_ConvertFToU_Ruchar_rtp( _T );
}

uchar OVERLOADABLE convert_uchar_rtn(uchar _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtn(ushort _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtn(uint _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtn(ulong _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtn(char _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtn(short _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtn(int _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtn(long _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtn(float _T) {
    return __spirv_ConvertFToU_Ruchar_rtn( _T );
}

uchar OVERLOADABLE convert_uchar_rtz(uchar _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtz(ushort _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtz(uint _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtz(ulong _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtz(char _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtz(short _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtz(int _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtz(long _T) {
  return convert_uchar(_T);
}

uchar OVERLOADABLE convert_uchar_rtz(float _T) {
    return __spirv_ConvertFToU_Ruchar_rtz( _T );
}

uchar OVERLOADABLE convert_uchar_sat(uchar _T) {
    return __spirv_UConvert_Ruchar_sat( _T );
}

uchar OVERLOADABLE convert_uchar_sat(ushort _T) {
    return __spirv_UConvert_Ruchar_sat( _T );
}

uchar OVERLOADABLE convert_uchar_sat(uint _T) {
    return __spirv_UConvert_Ruchar_sat( _T );
}

uchar OVERLOADABLE convert_uchar_sat(ulong _T) {
    return __spirv_UConvert_Ruchar_sat( _T );
}

uchar OVERLOADABLE convert_uchar_sat(char _T) {
    return __spirv_SatConvertSToU_Ruchar( _T );
}

uchar OVERLOADABLE convert_uchar_sat(short _T) {
    return __spirv_SatConvertSToU_Ruchar( _T );
}

uchar OVERLOADABLE convert_uchar_sat(int _T) {
    return __spirv_SatConvertSToU_Ruchar( _T );
}

uchar OVERLOADABLE convert_uchar_sat(long _T) {
    return __spirv_SatConvertSToU_Ruchar( _T );
}

uchar OVERLOADABLE convert_uchar_sat(float _T) {
    return __spirv_ConvertFToU_Ruchar_sat( _T );
}

uchar OVERLOADABLE convert_uchar_sat_rte(uchar _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rte(ushort _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rte(uint _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rte(ulong _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rte(char _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rte(short _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rte(int _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rte(long _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rte(float _T) {
    return __spirv_ConvertFToU_Ruchar_sat_rte( _T );
}

uchar OVERLOADABLE convert_uchar_sat_rtp(uchar _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtp(ushort _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtp(uint _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtp(ulong _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtp(char _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtp(short _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtp(int _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtp(long _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtp(float _T) {
    return __spirv_ConvertFToU_Ruchar_sat_rtp( _T );
}

uchar OVERLOADABLE convert_uchar_sat_rtn(uchar _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtn(ushort _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtn(uint _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtn(ulong _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtn(char _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtn(short _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtn(int _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtn(long _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtn(float _T) {
    return __spirv_ConvertFToU_Ruchar_sat_rtn( _T );
}

uchar OVERLOADABLE convert_uchar_sat_rtz(uchar _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtz(ushort _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtz(uint _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtz(ulong _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtz(char _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtz(short _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtz(int _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtz(long _T) {
  return convert_uchar_sat(_T);
}

uchar OVERLOADABLE convert_uchar_sat_rtz(float _T) {
    return __spirv_ConvertFToU_Ruchar_sat_rtz( _T );
}

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES( convert_uchar, uchar )
GENERATE_CONVERSIONS_FUNCTIONS_SAT_ALL_TYPES( convert_uchar, uchar )

ushort OVERLOADABLE convert_ushort(uchar _T) {
    return __spirv_UConvert_Rushort( _T );
}

ushort OVERLOADABLE convert_ushort(ushort _T) {
    return __spirv_UConvert_Rushort( _T );
}

ushort OVERLOADABLE convert_ushort(uint _T) {
    return __spirv_UConvert_Rushort( _T );
}

ushort OVERLOADABLE convert_ushort(ulong _T) {
    return __spirv_UConvert_Rushort( _T );
}

ushort OVERLOADABLE convert_ushort(char _T) {
    return __spirv_SConvert_Rshort( _T );
}

ushort OVERLOADABLE convert_ushort(short _T) {
    return __spirv_SConvert_Rshort( _T );
}

ushort OVERLOADABLE convert_ushort(int _T) {
    return __spirv_SConvert_Rshort( _T );
}

ushort OVERLOADABLE convert_ushort(long _T) {
    return __spirv_SConvert_Rshort( _T );
}

ushort OVERLOADABLE convert_ushort(float _T) {
    return __spirv_ConvertFToU_Rushort( _T );
}

ushort OVERLOADABLE convert_ushort_rte(uchar _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rte(ushort _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rte(uint _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rte(ulong _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rte(char _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rte(short _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rte(int _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rte(long _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rte(float _T) {
    return __spirv_ConvertFToU_Rushort_rte( _T );
}

ushort OVERLOADABLE convert_ushort_rtp(uchar _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtp(ushort _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtp(uint _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtp(ulong _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtp(char _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtp(short _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtp(int _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtp(long _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtp(float _T) {
    return __spirv_ConvertFToU_Rushort_rtp( _T );
}

ushort OVERLOADABLE convert_ushort_rtn(uchar _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtn(ushort _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtn(uint _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtn(ulong _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtn(char _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtn(short _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtn(int _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtn(long _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtn(float _T) {
    return __spirv_ConvertFToU_Rushort_rtn( _T );
}

ushort OVERLOADABLE convert_ushort_rtz(uchar _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtz(ushort _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtz(uint _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtz(ulong _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtz(char _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtz(short _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtz(int _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtz(long _T) {
  return convert_ushort(_T);
}

ushort OVERLOADABLE convert_ushort_rtz(float _T) {
    return __spirv_ConvertFToU_Rushort_rtz( _T );
}

ushort OVERLOADABLE convert_ushort_sat(uchar _T) {
    return __spirv_UConvert_Rushort_sat( _T );
}

ushort OVERLOADABLE convert_ushort_sat(ushort _T) {
    return __spirv_UConvert_Rushort_sat( _T );
}

ushort OVERLOADABLE convert_ushort_sat(uint _T) {
    return __spirv_UConvert_Rushort_sat( _T );
}

ushort OVERLOADABLE convert_ushort_sat(ulong _T) {
    return __spirv_UConvert_Rushort_sat( _T );
}

ushort OVERLOADABLE convert_ushort_sat(char _T) {
    return __spirv_SatConvertSToU_Rushort( _T );
}

ushort OVERLOADABLE convert_ushort_sat(short _T) {
    return __spirv_SatConvertSToU_Rushort( _T );
}

ushort OVERLOADABLE convert_ushort_sat(int _T) {
    return __spirv_SatConvertSToU_Rushort( _T );
}

ushort OVERLOADABLE convert_ushort_sat(long _T) {
    return __spirv_SatConvertSToU_Rushort( _T );
}

ushort OVERLOADABLE convert_ushort_sat(float _T) {
    return __spirv_ConvertFToU_Rushort_sat( _T );
}

ushort OVERLOADABLE convert_ushort_sat_rte(uchar _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rte(ushort _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rte(uint _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rte(ulong _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rte(char _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rte(short _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rte(int _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rte(long _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rte(float _T) {
    return __spirv_ConvertFToU_Rushort_sat_rte( _T );
}

ushort OVERLOADABLE convert_ushort_sat_rtp(uchar _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtp(ushort _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtp(uint _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtp(ulong _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtp(char _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtp(short _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtp(int _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtp(long _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtp(float _T) {
    return __spirv_ConvertFToU_Rushort_sat_rtp( _T );
}

ushort OVERLOADABLE convert_ushort_sat_rtn(uchar _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtn(ushort _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtn(uint _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtn(ulong _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtn(char _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtn(short _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtn(int _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtn(long _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtn(float _T) {
    return __spirv_ConvertFToU_Rushort_sat_rtn( _T );
}

ushort OVERLOADABLE convert_ushort_sat_rtz(uchar _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtz(ushort _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtz(uint _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtz(ulong _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtz(char _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtz(short _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtz(int _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtz(long _T) {
  return convert_ushort_sat(_T);
}

ushort OVERLOADABLE convert_ushort_sat_rtz(float _T) {
    return __spirv_ConvertFToU_Rushort_sat_rtz( _T );
}

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES( convert_ushort, ushort )
GENERATE_CONVERSIONS_FUNCTIONS_SAT_ALL_TYPES( convert_ushort, ushort )

uint OVERLOADABLE convert_uint(uchar _T) {
    return __spirv_UConvert_Ruint( _T );
}

uint OVERLOADABLE convert_uint(ushort _T) {
    return __spirv_UConvert_Ruint( _T );
}

uint OVERLOADABLE convert_uint(uint _T) {
    return __spirv_UConvert_Ruint( _T );
}

uint OVERLOADABLE convert_uint(ulong _T) {
    return __spirv_UConvert_Ruint( _T );
}

uint OVERLOADABLE convert_uint(char _T) {
    return __spirv_SConvert_Rint( _T );
}

uint OVERLOADABLE convert_uint(short _T) {
    return __spirv_SConvert_Rint( _T );
}

uint OVERLOADABLE convert_uint(int _T) {
    return __spirv_SConvert_Rint( _T );
}

uint OVERLOADABLE convert_uint(long _T) {
    return __spirv_SConvert_Rint( _T );
}

uint OVERLOADABLE convert_uint_rte(uchar _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rte(ushort _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rte(uint _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rte(ulong _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rte(char _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rte(short _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rte(int _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rte(long _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtp(uchar _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtp(ushort _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtp(uint _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtp(ulong _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtp(char _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtp(short _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtp(int _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtp(long _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtn(uchar _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtn(ushort _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtn(uint _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtn(ulong _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtn(char _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtn(short _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtn(int _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtn(long _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtz(uchar _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtz(ushort _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtz(uint _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtz(ulong _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtz(char _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtz(short _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtz(int _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_rtz(long _T) {
  return convert_uint(_T);
}

uint OVERLOADABLE convert_uint_sat(uchar _T) {
    return __spirv_UConvert_Ruint_sat( _T );
}

uint OVERLOADABLE convert_uint_sat(ushort _T) {
    return __spirv_UConvert_Ruint_sat( _T );
}

uint OVERLOADABLE convert_uint_sat(uint _T) {
    return __spirv_UConvert_Ruint_sat( _T );
}

uint OVERLOADABLE convert_uint_sat(ulong _T) {
    return __spirv_UConvert_Ruint_sat( _T );
}

uint OVERLOADABLE convert_uint_sat(char _T) {
    return __spirv_SatConvertSToU_Ruint( _T );
}

uint OVERLOADABLE convert_uint_sat(short _T) {
    return __spirv_SatConvertSToU_Ruint( _T );
}

uint OVERLOADABLE convert_uint_sat(int _T) {
    return __spirv_SatConvertSToU_Ruint( _T );
}

uint OVERLOADABLE convert_uint_sat(long _T) {
    return __spirv_SatConvertSToU_Ruint( _T );
}

uint OVERLOADABLE convert_uint_sat_rte(uchar _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rte(ushort _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rte(uint _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rte(ulong _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rte(char _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rte(short _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rte(int _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rte(long _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtp(uchar _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtp(ushort _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtp(uint _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtp(ulong _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtp(char _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtp(short _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtp(int _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtp(long _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtn(uchar _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtn(ushort _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtn(uint _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtn(ulong _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtn(char _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtn(short _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtn(int _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtn(long _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtz(uchar _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtz(ushort _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtz(uint _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtz(ulong _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtz(char _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtz(short _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtz(int _T) {
  return convert_uint_sat(_T);
}

uint OVERLOADABLE convert_uint_sat_rtz(long _T) {
  return convert_uint_sat(_T);
}

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES( convert_uint, uint )
GENERATE_CONVERSIONS_FUNCTIONS_SAT_ALL_TYPES( convert_uint, uint )

ulong OVERLOADABLE convert_ulong(uchar _T) {
    return __spirv_UConvert_Rulong( _T );
}

ulong OVERLOADABLE convert_ulong(ushort _T) {
    return __spirv_UConvert_Rulong( _T );
}

ulong OVERLOADABLE convert_ulong(uint _T) {
    return __spirv_UConvert_Rulong( _T );
}

ulong OVERLOADABLE convert_ulong(ulong _T) {
    return __spirv_UConvert_Rulong( _T );
}

ulong OVERLOADABLE convert_ulong(char _T) {
    return __spirv_SConvert_Rlong( _T );
}

ulong OVERLOADABLE convert_ulong(short _T) {
    return __spirv_SConvert_Rlong( _T );
}

ulong OVERLOADABLE convert_ulong(int _T) {
    return __spirv_SConvert_Rlong( _T );
}

ulong OVERLOADABLE convert_ulong(long _T) {
    return __spirv_SConvert_Rlong( _T );
}

ulong OVERLOADABLE convert_ulong_rte(uchar _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rte(ushort _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rte(uint _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rte(ulong _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rte(char _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rte(short _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rte(int _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rte(long _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtp(uchar _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtp(ushort _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtp(uint _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtp(ulong _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtp(char _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtp(short _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtp(int _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtp(long _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtn(uchar _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtn(ushort _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtn(uint _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtn(ulong _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtn(char _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtn(short _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtn(int _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtn(long _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtz(uchar _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtz(ushort _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtz(uint _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtz(ulong _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtz(char _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtz(short _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtz(int _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_rtz(long _T) {
  return convert_ulong(_T);
}

ulong OVERLOADABLE convert_ulong_sat(uchar _T) {
    return __spirv_UConvert_Rulong_sat( _T );
}

ulong OVERLOADABLE convert_ulong_sat(ushort _T) {
    return __spirv_UConvert_Rulong_sat( _T );
}

ulong OVERLOADABLE convert_ulong_sat(uint _T) {
    return __spirv_UConvert_Rulong_sat( _T );
}

ulong OVERLOADABLE convert_ulong_sat(ulong _T) {
    return __spirv_UConvert_Rulong_sat( _T );
}

ulong OVERLOADABLE convert_ulong_sat(char _T) {
    return __spirv_SatConvertSToU_Rulong( _T );
}

ulong OVERLOADABLE convert_ulong_sat(short _T) {
    return __spirv_SatConvertSToU_Rulong( _T );
}

ulong OVERLOADABLE convert_ulong_sat(int _T) {
    return __spirv_SatConvertSToU_Rulong( _T );
}

ulong OVERLOADABLE convert_ulong_sat(long _T) {
    return __spirv_SatConvertSToU_Rulong( _T );
}

ulong OVERLOADABLE convert_ulong_sat_rte(uchar _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rte(ushort _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rte(uint _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rte(ulong _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rte(char _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rte(short _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rte(int _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rte(long _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtp(uchar _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtp(ushort _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtp(uint _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtp(ulong _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtp(char _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtp(short _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtp(int _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtp(long _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtn(uchar _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtn(ushort _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtn(uint _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtn(ulong _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtn(char _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtn(short _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtn(int _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtn(long _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtz(uchar _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtz(ushort _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtz(uint _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtz(ulong _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtz(char _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtz(short _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtz(int _T) {
  return convert_ulong_sat(_T);
}

ulong OVERLOADABLE convert_ulong_sat_rtz(long _T) {
  return convert_ulong_sat(_T);
}

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES( convert_ulong, ulong )
GENERATE_CONVERSIONS_FUNCTIONS_SAT_ALL_TYPES( convert_ulong, ulong )

char OVERLOADABLE convert_char(uchar _T) {
    return __spirv_UConvert_Ruchar( _T );
}

char OVERLOADABLE convert_char(ushort _T) {
    return __spirv_UConvert_Ruchar( _T );
}

char OVERLOADABLE convert_char(uint _T) {
    return __spirv_UConvert_Ruchar( _T );
}

char OVERLOADABLE convert_char(ulong _T) {
    return __spirv_UConvert_Ruchar( _T );
}

char OVERLOADABLE convert_char(char _T) {
    return __spirv_SConvert_Rchar( _T );
}

char OVERLOADABLE convert_char(short _T) {
    return __spirv_SConvert_Rchar( _T );
}

char OVERLOADABLE convert_char(int _T) {
    return __spirv_SConvert_Rchar( _T );
}

char OVERLOADABLE convert_char(long _T) {
    return __spirv_SConvert_Rchar( _T );
}

char OVERLOADABLE convert_char(float _T) {
    return __spirv_ConvertFToS_Rchar( _T );
}

char OVERLOADABLE convert_char_rte(uchar _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rte(ushort _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rte(uint _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rte(ulong _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rte(char _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rte(short _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rte(int _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rte(long _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rte(float _T) {
    return __spirv_ConvertFToS_Rchar_rte( _T );
}

char OVERLOADABLE convert_char_rtp(uchar _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtp(ushort _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtp(uint _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtp(ulong _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtp(char _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtp(short _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtp(int _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtp(long _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtp(float _T) {
    return __spirv_ConvertFToS_Rchar_rtp( _T );
}

char OVERLOADABLE convert_char_rtn(uchar _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtn(ushort _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtn(uint _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtn(ulong _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtn(char _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtn(short _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtn(int _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtn(long _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtn(float _T) {
    return __spirv_ConvertFToS_Rchar_rtn( _T );
}

char OVERLOADABLE convert_char_rtz(uchar _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtz(ushort _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtz(uint _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtz(ulong _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtz(char _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtz(short _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtz(int _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtz(long _T) {
  return convert_char(_T);
}

char OVERLOADABLE convert_char_rtz(float _T) {
    return __spirv_ConvertFToS_Rchar_rtz( _T );
}

char OVERLOADABLE convert_char_sat(uchar _T) {
    return __spirv_SatConvertUToS_Rchar( _T );
}

char OVERLOADABLE convert_char_sat(ushort _T) {
    return __spirv_SatConvertUToS_Rchar( _T );
}

char OVERLOADABLE convert_char_sat(uint _T) {
    return __spirv_SatConvertUToS_Rchar( _T );
}

char OVERLOADABLE convert_char_sat(ulong _T) {
    return __spirv_SatConvertUToS_Rchar( _T );
}

char OVERLOADABLE convert_char_sat(char _T) {
    return __spirv_SConvert_Rchar_sat( _T );
}

char OVERLOADABLE convert_char_sat(short _T) {
    return __spirv_SConvert_Rchar_sat( _T );
}

char OVERLOADABLE convert_char_sat(int _T) {
    return __spirv_SConvert_Rchar_sat( _T );
}

char OVERLOADABLE convert_char_sat(long _T) {
    return __spirv_SConvert_Rchar_sat( _T );
}

char OVERLOADABLE convert_char_sat(float _T) {
    return __spirv_ConvertFToS_Rchar_sat( _T );
}

char OVERLOADABLE convert_char_sat_rte(uchar _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rte(ushort _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rte(uint _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rte(ulong _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rte(char _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rte(short _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rte(int _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rte(long _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rte(float _T) {
    return __spirv_ConvertFToS_Rchar_sat_rte( _T );
}

char OVERLOADABLE convert_char_sat_rtp(uchar _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtp(ushort _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtp(uint _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtp(ulong _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtp(char _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtp(short _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtp(int _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtp(long _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtp(float _T) {
    return __spirv_ConvertFToS_Rchar_sat_rtp( _T );
}

char OVERLOADABLE convert_char_sat_rtn(uchar _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtn(ushort _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtn(uint _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtn(ulong _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtn(char _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtn(short _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtn(int _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtn(long _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtn(float _T) {
    return __spirv_ConvertFToS_Rchar_sat_rtn( _T );
}

char OVERLOADABLE convert_char_sat_rtz(uchar _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtz(ushort _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtz(uint _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtz(ulong _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtz(char _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtz(short _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtz(int _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtz(long _T) {
  return convert_char_sat(_T);
}

char OVERLOADABLE convert_char_sat_rtz(float _T) {
    return __spirv_ConvertFToS_Rchar_sat_rtz( _T );
}

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES( convert_char, char )
GENERATE_CONVERSIONS_FUNCTIONS_SAT_ALL_TYPES( convert_char, char )

short OVERLOADABLE convert_short(uchar _T) {
    return __spirv_UConvert_Rushort( _T );
}

short OVERLOADABLE convert_short(ushort _T) {
    return __spirv_UConvert_Rushort( _T );
}

short OVERLOADABLE convert_short(uint _T) {
    return __spirv_UConvert_Rushort( _T );
}

short OVERLOADABLE convert_short(ulong _T) {
    return __spirv_UConvert_Rushort( _T );
}

short OVERLOADABLE convert_short(char _T) {
    return __spirv_SConvert_Rshort( _T );
}

short OVERLOADABLE convert_short(short _T) {
    return __spirv_SConvert_Rshort( _T );
}

short OVERLOADABLE convert_short(int _T) {
    return __spirv_SConvert_Rshort( _T );
}

short OVERLOADABLE convert_short(long _T) {
    return __spirv_SConvert_Rshort( _T );
}

short OVERLOADABLE convert_short(float _T) {
    return __spirv_ConvertFToS_Rshort( _T );
}

short OVERLOADABLE convert_short_rte(uchar _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rte(ushort _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rte(uint _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rte(ulong _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rte(char _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rte(short _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rte(int _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rte(long _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rte(float _T) {
    return __spirv_ConvertFToS_Rshort_rte( _T );
}

short OVERLOADABLE convert_short_rtp(uchar _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtp(ushort _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtp(uint _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtp(ulong _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtp(char _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtp(short _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtp(int _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtp(long _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtp(float _T) {
    return __spirv_ConvertFToS_Rshort_rtp( _T );
}

short OVERLOADABLE convert_short_rtn(uchar _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtn(ushort _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtn(uint _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtn(ulong _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtn(char _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtn(short _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtn(int _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtn(long _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtn(float _T) {
    return __spirv_ConvertFToS_Rshort_rtn( _T );
}

short OVERLOADABLE convert_short_rtz(uchar _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtz(ushort _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtz(uint _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtz(ulong _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtz(char _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtz(short _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtz(int _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtz(long _T) {
  return convert_short(_T);
}

short OVERLOADABLE convert_short_rtz(float _T) {
    return __spirv_ConvertFToS_Rshort_rtz( _T );
}

short OVERLOADABLE convert_short_sat(uchar _T) {
    return __spirv_SatConvertUToS_Rshort( _T );
}

short OVERLOADABLE convert_short_sat(ushort _T) {
    return __spirv_SatConvertUToS_Rshort( _T );
}

short OVERLOADABLE convert_short_sat(uint _T) {
    return __spirv_SatConvertUToS_Rshort( _T );
}

short OVERLOADABLE convert_short_sat(ulong _T) {
    return __spirv_SatConvertUToS_Rshort( _T );
}

short OVERLOADABLE convert_short_sat(char _T) {
    return __spirv_SConvert_Rshort_sat( _T );
}

short OVERLOADABLE convert_short_sat(short _T) {
    return __spirv_SConvert_Rshort_sat( _T );
}

short OVERLOADABLE convert_short_sat(int _T) {
    return __spirv_SConvert_Rshort_sat( _T );
}

short OVERLOADABLE convert_short_sat(long _T) {
    return __spirv_SConvert_Rshort_sat( _T );
}

short OVERLOADABLE convert_short_sat(float _T) {
    return __spirv_ConvertFToS_Rshort_sat( _T );
}

short OVERLOADABLE convert_short_sat_rte(uchar _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rte(ushort _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rte(uint _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rte(ulong _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rte(char _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rte(short _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rte(int _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rte(long _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rte(float _T) {
    return __spirv_ConvertFToS_Rshort_sat_rte( _T );
}

short OVERLOADABLE convert_short_sat_rtp(uchar _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtp(ushort _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtp(uint _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtp(ulong _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtp(char _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtp(short _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtp(int _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtp(long _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtp(float _T) {
    return __spirv_ConvertFToS_Rshort_sat_rtp( _T );
}

short OVERLOADABLE convert_short_sat_rtn(uchar _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtn(ushort _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtn(uint _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtn(ulong _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtn(char _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtn(short _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtn(int _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtn(long _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtn(float _T) {
    return __spirv_ConvertFToS_Rshort_sat_rtn( _T );
}

short OVERLOADABLE convert_short_sat_rtz(uchar _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtz(ushort _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtz(uint _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtz(ulong _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtz(char _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtz(short _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtz(int _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtz(long _T) {
  return convert_short_sat(_T);
}

short OVERLOADABLE convert_short_sat_rtz(float _T) {
    return __spirv_ConvertFToS_Rshort_sat_rtz( _T );
}

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES( convert_short, short )
GENERATE_CONVERSIONS_FUNCTIONS_SAT_ALL_TYPES( convert_short, short )

int OVERLOADABLE convert_int(uchar _T) {
    return __spirv_UConvert_Ruint( _T );
}

int OVERLOADABLE convert_int(ushort _T) {
    return __spirv_UConvert_Ruint( _T );
}

int OVERLOADABLE convert_int(uint _T) {
    return __spirv_UConvert_Ruint( _T );
}

int OVERLOADABLE convert_int(ulong _T) {
    return __spirv_UConvert_Ruint( _T );
}

int OVERLOADABLE convert_int(char _T) {
    return __spirv_SConvert_Rint( _T );
}

int OVERLOADABLE convert_int(short _T) {
    return __spirv_SConvert_Rint( _T );
}

int OVERLOADABLE convert_int(int _T) {
    return __spirv_SConvert_Rint( _T );
}

int OVERLOADABLE convert_int(long _T) {
    return __spirv_SConvert_Rint( _T );
}

int OVERLOADABLE convert_int_rte(uchar _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rte(ushort _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rte(uint _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rte(ulong _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rte(char _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rte(short _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rte(int _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rte(long _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtp(uchar _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtp(ushort _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtp(uint _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtp(ulong _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtp(char _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtp(short _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtp(int _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtp(long _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtn(uchar _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtn(ushort _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtn(uint _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtn(ulong _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtn(char _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtn(short _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtn(int _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtn(long _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtz(uchar _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtz(ushort _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtz(uint _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtz(ulong _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtz(char _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtz(short _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtz(int _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_rtz(long _T) {
  return convert_int(_T);
}

int OVERLOADABLE convert_int_sat(uchar _T) {
    return __spirv_SatConvertUToS_Rint( _T );
}

int OVERLOADABLE convert_int_sat(ushort _T) {
    return __spirv_SatConvertUToS_Rint( _T );
}

int OVERLOADABLE convert_int_sat(uint _T) {
    return __spirv_SatConvertUToS_Rint( _T );
}

int OVERLOADABLE convert_int_sat(ulong _T) {
    return __spirv_SatConvertUToS_Rint( _T );
}

int OVERLOADABLE convert_int_sat(char _T) {
    return __spirv_SConvert_Rint_sat( _T );
}

int OVERLOADABLE convert_int_sat(short _T) {
    return __spirv_SConvert_Rint_sat( _T );
}

int OVERLOADABLE convert_int_sat(int _T) {
    return __spirv_SConvert_Rint_sat( _T );
}

int OVERLOADABLE convert_int_sat(long _T) {
    return __spirv_SConvert_Rint_sat( _T );
}

int OVERLOADABLE convert_int_sat_rte(uchar _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rte(ushort _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rte(uint _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rte(ulong _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rte(char _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rte(short _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rte(int _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rte(long _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtp(uchar _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtp(ushort _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtp(uint _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtp(ulong _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtp(char _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtp(short _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtp(int _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtp(long _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtn(uchar _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtn(ushort _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtn(uint _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtn(ulong _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtn(char _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtn(short _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtn(int _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtn(long _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtz(uchar _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtz(ushort _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtz(uint _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtz(ulong _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtz(char _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtz(short _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtz(int _T) {
  return convert_int_sat(_T);
}

int OVERLOADABLE convert_int_sat_rtz(long _T) {
  return convert_int_sat(_T);
}

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES( convert_int, int )
GENERATE_CONVERSIONS_FUNCTIONS_SAT_ALL_TYPES( convert_int, int )

long OVERLOADABLE convert_long(uchar _T) {
    return __spirv_UConvert_Rulong( _T );
}

long OVERLOADABLE convert_long(ushort _T) {
    return __spirv_UConvert_Rulong( _T );
}

long OVERLOADABLE convert_long(uint _T) {
    return __spirv_UConvert_Rulong( _T );
}

long OVERLOADABLE convert_long(ulong _T) {
    return __spirv_UConvert_Rulong( _T );
}

long OVERLOADABLE convert_long(char _T) {
    return __spirv_SConvert_Rlong( _T );
}

long OVERLOADABLE convert_long(short _T) {
    return __spirv_SConvert_Rlong( _T );
}

long OVERLOADABLE convert_long(int _T) {
    return __spirv_SConvert_Rlong( _T );
}

long OVERLOADABLE convert_long(long _T) {
    return __spirv_SConvert_Rlong( _T );
}

long OVERLOADABLE convert_long_rte(uchar _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rte(ushort _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rte(uint _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rte(ulong _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rte(char _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rte(short _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rte(int _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rte(long _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtp(uchar _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtp(ushort _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtp(uint _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtp(ulong _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtp(char _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtp(short _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtp(int _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtp(long _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtn(uchar _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtn(ushort _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtn(uint _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtn(ulong _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtn(char _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtn(short _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtn(int _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtn(long _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtz(uchar _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtz(ushort _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtz(uint _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtz(ulong _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtz(char _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtz(short _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtz(int _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_rtz(long _T) {
  return convert_long(_T);
}

long OVERLOADABLE convert_long_sat(uchar _T) {
    return __spirv_SatConvertUToS_Rlong( _T );
}

long OVERLOADABLE convert_long_sat(ushort _T) {
    return __spirv_SatConvertUToS_Rlong( _T );
}

long OVERLOADABLE convert_long_sat(uint _T) {
    return __spirv_SatConvertUToS_Rlong( _T );
}

long OVERLOADABLE convert_long_sat(ulong _T) {
    return __spirv_SatConvertUToS_Rlong( _T );
}

long OVERLOADABLE convert_long_sat(char _T) {
    return __spirv_SConvert_Rlong_sat( _T );
}

long OVERLOADABLE convert_long_sat(short _T) {
    return __spirv_SConvert_Rlong_sat( _T );
}

long OVERLOADABLE convert_long_sat(int _T) {
    return __spirv_SConvert_Rlong_sat( _T );
}

long OVERLOADABLE convert_long_sat(long _T) {
    return __spirv_SConvert_Rlong_sat( _T );
}

long OVERLOADABLE convert_long_sat_rte(uchar _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rte(ushort _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rte(uint _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rte(ulong _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rte(char _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rte(short _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rte(int _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rte(long _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtp(uchar _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtp(ushort _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtp(uint _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtp(ulong _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtp(char _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtp(short _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtp(int _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtp(long _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtn(uchar _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtn(ushort _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtn(uint _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtn(ulong _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtn(char _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtn(short _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtn(int _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtn(long _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtz(uchar _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtz(ushort _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtz(uint _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtz(ulong _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtz(char _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtz(short _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtz(int _T) {
  return convert_long_sat(_T);
}

long OVERLOADABLE convert_long_sat_rtz(long _T) {
  return convert_long_sat(_T);
}

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES( convert_long, long )
GENERATE_CONVERSIONS_FUNCTIONS_SAT_ALL_TYPES( convert_long, long )

float OVERLOADABLE convert_float(uchar _T) {
    return __spirv_ConvertUToF_Rfloat( _T );
}

float OVERLOADABLE convert_float(ushort _T) {
    return __spirv_ConvertUToF_Rfloat( _T );
}

float OVERLOADABLE convert_float(char _T) {
    return __spirv_ConvertSToF_Rfloat( _T );
}

float OVERLOADABLE convert_float(short _T) {
    return __spirv_ConvertSToF_Rfloat( _T );
}

float OVERLOADABLE convert_float_rte(uchar _T) {
    return __spirv_ConvertUToF_Rfloat_rte( _T );
}

float OVERLOADABLE convert_float_rte(ushort _T) {
    return __spirv_ConvertUToF_Rfloat_rte( _T );
}

float OVERLOADABLE convert_float_rte(char _T) {
    return __spirv_ConvertSToF_Rfloat_rte( _T );
}

float OVERLOADABLE convert_float_rte(short _T) {
    return __spirv_ConvertSToF_Rfloat_rte( _T );
}

float OVERLOADABLE convert_float_rtp(uchar _T) {
    return __spirv_ConvertUToF_Rfloat_rtp( _T );
}

float OVERLOADABLE convert_float_rtp(ushort _T) {
    return __spirv_ConvertUToF_Rfloat_rtp( _T );
}

float OVERLOADABLE convert_float_rtp(char _T) {
    return __spirv_ConvertSToF_Rfloat_rtp( _T );
}

float OVERLOADABLE convert_float_rtp(short _T) {
    return __spirv_ConvertSToF_Rfloat_rtp( _T );
}

float OVERLOADABLE convert_float_rtn(uchar _T) {
    return __spirv_ConvertUToF_Rfloat_rtn( _T );
}

float OVERLOADABLE convert_float_rtn(ushort _T) {
    return __spirv_ConvertUToF_Rfloat_rtn( _T );
}

float OVERLOADABLE convert_float_rtn(char _T) {
    return __spirv_ConvertSToF_Rfloat_rtn( _T );
}

float OVERLOADABLE convert_float_rtn(short _T) {
    return __spirv_ConvertSToF_Rfloat_rtn( _T );
}

float OVERLOADABLE convert_float_rtz(uchar _T) {
    return __spirv_ConvertUToF_Rfloat_rtz( _T );
}

float OVERLOADABLE convert_float_rtz(ushort _T) {
    return __spirv_ConvertUToF_Rfloat_rtz( _T );
}

float OVERLOADABLE convert_float_rtz(char _T) {
    return __spirv_ConvertSToF_Rfloat_rtz( _T );
}

float OVERLOADABLE convert_float_rtz(short _T) {
    return __spirv_ConvertSToF_Rfloat_rtz( _T );
}

float OVERLOADABLE convert_float(float _T) {
    return __spirv_FConvert_Rfloat( _T );
}

float OVERLOADABLE convert_float_rte(float _T) {
    return __spirv_FConvert_Rfloat_rte( _T );
}

float OVERLOADABLE convert_float_rtn(float _T) {
    return __spirv_FConvert_Rfloat_rtn( _T );
}

float OVERLOADABLE convert_float_rtp(float _T) {
    return __spirv_FConvert_Rfloat_rtp( _T );
}

float OVERLOADABLE convert_float_rtz(float _T) {
    return __spirv_FConvert_Rfloat_rtz( _T );
}

int OVERLOADABLE convert_int(float _T) {
    return __spirv_ConvertFToS_Rint( _T );
}

int OVERLOADABLE convert_int_rte(float _T) {
    return __spirv_ConvertFToS_Rint_rte( _T );
}

int OVERLOADABLE convert_int_rtn(float _T) {
    return __spirv_ConvertFToS_Rint_rtn( _T );
}

int OVERLOADABLE convert_int_rtp(float _T) {
    return __spirv_ConvertFToS_Rint_rtp( _T );
}

int OVERLOADABLE convert_int_rtz(float _T) {
    return __spirv_ConvertFToS_Rint_rtz( _T );
}

int OVERLOADABLE convert_int_sat(float _T) {
    return __spirv_ConvertFToS_Rint_sat( _T );
}

int OVERLOADABLE convert_int_sat_rte(float _T) {
    return __spirv_ConvertFToS_Rint_sat_rte( _T );
}

int OVERLOADABLE convert_int_sat_rtn(float _T) {
    return __spirv_ConvertFToS_Rint_sat_rtn( _T );
}

int OVERLOADABLE convert_int_sat_rtp(float _T) {
    return __spirv_ConvertFToS_Rint_sat_rtp( _T );
}

int OVERLOADABLE convert_int_sat_rtz(float _T) {
    return __spirv_ConvertFToS_Rint_sat_rtz( _T );
}

uint OVERLOADABLE convert_uint(float _T) {
    return __spirv_ConvertFToU_Ruint( _T );
}

uint OVERLOADABLE convert_uint_rte(float _T) {
    return __spirv_ConvertFToU_Ruint_rte( _T );
}

uint OVERLOADABLE convert_uint_rtn(float _T) {
    return __spirv_ConvertFToU_Ruint_rtn( _T );
}

uint OVERLOADABLE convert_uint_rtp(float _T) {
    return __spirv_ConvertFToU_Ruint_rtp( _T );
}

uint OVERLOADABLE convert_uint_rtz(float _T) {
    return __spirv_ConvertFToU_Ruint_rtz( _T );
}

uint OVERLOADABLE convert_uint_sat(float _T) {
    return __spirv_ConvertFToU_Ruint_sat( _T );
}

uint OVERLOADABLE convert_uint_sat_rte(float _T) {
    return __spirv_ConvertFToU_Ruint_sat_rte( _T );
}

uint OVERLOADABLE convert_uint_sat_rtn(float _T) {
    return __spirv_ConvertFToU_Ruint_sat_rtn( _T );
}

uint OVERLOADABLE convert_uint_sat_rtp(float _T) {
    return __spirv_ConvertFToU_Ruint_sat_rtp( _T );
}

uint OVERLOADABLE convert_uint_sat_rtz(float _T) {
    return __spirv_ConvertFToU_Ruint_sat_rtz( _T );
}

long OVERLOADABLE convert_long_rte(float _T) {
    return __spirv_ConvertFToS_Rlong_rte( _T );
}

long OVERLOADABLE convert_long_rtn(float _T) {
    return __spirv_ConvertFToS_Rlong_rtn( _T );
}

long OVERLOADABLE convert_long_rtp(float _T) {
    return __spirv_ConvertFToS_Rlong_rtp( _T );
}

long OVERLOADABLE convert_long_rtz(float _T) {
    return __spirv_ConvertFToS_Rlong_rtz( _T );
}

long OVERLOADABLE convert_long_sat(float _T) {
    return __spirv_ConvertFToS_Rlong_sat( _T );
}

long OVERLOADABLE convert_long_sat_rte(float _T) {
    return __spirv_ConvertFToS_Rlong_sat_rte( _T );
}

long OVERLOADABLE convert_long_sat_rtn(float _T) {
    return __spirv_ConvertFToS_Rlong_sat_rtn( _T );
}

long OVERLOADABLE convert_long_sat_rtp(float _T) {
    return __spirv_ConvertFToS_Rlong_sat_rtp( _T );
}

long OVERLOADABLE convert_long_sat_rtz(float _T) {
    return __spirv_ConvertFToS_Rlong_sat_rtz( _T );
}

ulong OVERLOADABLE convert_ulong_rte(float _T) {
    return __spirv_ConvertFToU_Rulong_rte( _T );
}

ulong OVERLOADABLE convert_ulong_rtn(float _T) {
    return __spirv_ConvertFToU_Rulong_rtn( _T );
}

ulong OVERLOADABLE convert_ulong_rtp(float _T) {
    return __spirv_ConvertFToU_Rulong_rtp( _T );
}

ulong OVERLOADABLE convert_ulong_rtz(float _T) {
    return __spirv_ConvertFToU_Rulong_rtz( _T );
}

ulong OVERLOADABLE convert_ulong_sat(float _T) {
    return __spirv_ConvertFToU_Rulong_sat( _T );
}

ulong OVERLOADABLE convert_ulong_sat_rte(float _T) {
    return __spirv_ConvertFToU_Rulong_sat_rte( _T );
}

ulong OVERLOADABLE convert_ulong_sat_rtn(float _T) {
    return __spirv_ConvertFToU_Rulong_sat_rtn( _T );
}

ulong OVERLOADABLE convert_ulong_sat_rtp(float _T) {
    return __spirv_ConvertFToU_Rulong_sat_rtp( _T );
}

ulong OVERLOADABLE convert_ulong_sat_rtz(float _T) {
    return __spirv_ConvertFToU_Rulong_sat_rtz( _T );
}

float OVERLOADABLE convert_float(int _T) {
    return __spirv_ConvertSToF_Rfloat( _T );
}

float OVERLOADABLE convert_float_rte(int _T) {
    return __spirv_ConvertSToF_Rfloat_rte( _T );
}

float OVERLOADABLE convert_float_rtn(int _T) {
    return __spirv_ConvertSToF_Rfloat_rtn( _T );
}

float OVERLOADABLE convert_float_rtp(int _T) {
    return __spirv_ConvertSToF_Rfloat_rtp( _T );
}

float OVERLOADABLE convert_float_rtz(int _T) {
    return __spirv_ConvertSToF_Rfloat_rtz( _T );
}

float OVERLOADABLE convert_float(uint _T) {
    return __spirv_ConvertUToF_Rfloat( _T );
}

float OVERLOADABLE convert_float_rte(uint _T) {
    return __spirv_ConvertUToF_Rfloat_rte( _T );
}

float OVERLOADABLE convert_float_rtn(uint _T) {
    return __spirv_ConvertUToF_Rfloat_rtn( _T );
}

float OVERLOADABLE convert_float_rtp(uint _T) {
    return __spirv_ConvertUToF_Rfloat_rtp( _T );
}

float OVERLOADABLE convert_float_rtz(uint _T) {
    return __spirv_ConvertUToF_Rfloat_rtz( _T );
}

float OVERLOADABLE convert_float_rte(long _T) {
    return __spirv_ConvertSToF_Rfloat_rte( _T );
}

float OVERLOADABLE convert_float_rte(ulong _T) {
    return __spirv_ConvertUToF_Rfloat_rte( _T );
}

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES( convert_float, float )

#if defined(cl_khr_fp16)

uchar OVERLOADABLE convert_uchar(half _T) {
    return __spirv_ConvertFToU_Ruchar( _T );
}

uchar OVERLOADABLE convert_uchar_rte(half _T) {
    return __spirv_ConvertFToU_Ruchar_rte( _T );
}

uchar OVERLOADABLE convert_uchar_rtp(half _T) {
    return __spirv_ConvertFToU_Ruchar_rtp( _T );
}

uchar OVERLOADABLE convert_uchar_rtn(half _T) {
    return __spirv_ConvertFToU_Ruchar_rtn( _T );
}

uchar OVERLOADABLE convert_uchar_rtz(half _T) {
    return __spirv_ConvertFToU_Ruchar_rtz( _T );
}

uchar OVERLOADABLE convert_uchar_sat(half _T) {
    return __spirv_ConvertFToU_Ruchar_sat( _T );
}

uchar OVERLOADABLE convert_uchar_sat_rte(half _T) {
    return __spirv_ConvertFToU_Ruchar_sat_rte( _T );
}

uchar OVERLOADABLE convert_uchar_sat_rtp(half _T) {
    return __spirv_ConvertFToU_Ruchar_sat_rtp( _T );
}

uchar OVERLOADABLE convert_uchar_sat_rtn(half _T) {
    return __spirv_ConvertFToU_Ruchar_sat_rtn( _T );
}

uchar OVERLOADABLE convert_uchar_sat_rtz(half _T) {
    return __spirv_ConvertFToU_Ruchar_sat_rtz( _T );
}

ushort OVERLOADABLE convert_ushort(half _T) {
    return __spirv_ConvertFToU_Rushort( _T );
}

ushort OVERLOADABLE convert_ushort_rte(half _T) {
    return __spirv_ConvertFToU_Rushort_rte( _T );
}

ushort OVERLOADABLE convert_ushort_rtp(half _T) {
    return __spirv_ConvertFToU_Rushort_rtp( _T );
}

ushort OVERLOADABLE convert_ushort_rtn(half _T) {
    return __spirv_ConvertFToU_Rushort_rtn( _T );
}

ushort OVERLOADABLE convert_ushort_rtz(half _T) {
    return __spirv_ConvertFToU_Rushort_rtz( _T );
}

ushort OVERLOADABLE convert_ushort_sat(half _T) {
    return __spirv_ConvertFToU_Rushort_sat( _T );
}

ushort OVERLOADABLE convert_ushort_sat_rte(half _T) {
    return __spirv_ConvertFToU_Rushort_sat_rte( _T );
}

ushort OVERLOADABLE convert_ushort_sat_rtp(half _T) {
    return __spirv_ConvertFToU_Rushort_sat_rtp( _T );
}

ushort OVERLOADABLE convert_ushort_sat_rtn(half _T) {
    return __spirv_ConvertFToU_Rushort_sat_rtn( _T );
}

ushort OVERLOADABLE convert_ushort_sat_rtz(half _T) {
    return __spirv_ConvertFToU_Rushort_sat_rtz( _T );
}

uint OVERLOADABLE convert_uint(half _T) {
    return __spirv_ConvertFToU_Ruint( _T );
}

uint OVERLOADABLE convert_uint_rte(half _T) {
    return __spirv_ConvertFToU_Ruint_rte( _T );
}

uint OVERLOADABLE convert_uint_rtp(half _T) {
    return __spirv_ConvertFToU_Ruint_rtp( _T );
}

uint OVERLOADABLE convert_uint_rtn(half _T) {
    return __spirv_ConvertFToU_Ruint_rtn( _T );
}

uint OVERLOADABLE convert_uint_rtz(half _T) {
    return __spirv_ConvertFToU_Ruint_rtz( _T );
}

uint OVERLOADABLE convert_uint_sat(half _T) {
    return __spirv_ConvertFToU_Ruint_sat( _T );
}

uint OVERLOADABLE convert_uint_sat_rte(half _T) {
    return __spirv_ConvertFToU_Ruint_sat_rte( _T );
}

uint OVERLOADABLE convert_uint_sat_rtp(half _T) {
    return __spirv_ConvertFToU_Ruint_sat_rtp( _T );
}

uint OVERLOADABLE convert_uint_sat_rtn(half _T) {
    return __spirv_ConvertFToU_Ruint_sat_rtn( _T );
}

uint OVERLOADABLE convert_uint_sat_rtz(half _T) {
    return __spirv_ConvertFToU_Ruint_sat_rtz( _T );
}

ulong OVERLOADABLE convert_ulong(half _T) {
    return __spirv_ConvertFToU_Rulong( _T );
}

ulong OVERLOADABLE convert_ulong_rte(half _T) {
    return __spirv_ConvertFToU_Rulong_rte( _T );
}

ulong OVERLOADABLE convert_ulong_rtp(half _T) {
    return __spirv_ConvertFToU_Rulong_rtp( _T );
}

ulong OVERLOADABLE convert_ulong_rtn(half _T) {
    return __spirv_ConvertFToU_Rulong_rtn( _T );
}

ulong OVERLOADABLE convert_ulong_rtz(half _T) {
    return __spirv_ConvertFToU_Rulong_rtz( _T );
}

ulong OVERLOADABLE convert_ulong_sat(half _T) {
    return __spirv_ConvertFToU_Rulong_sat( _T );
}

ulong OVERLOADABLE convert_ulong_sat_rte(half _T) {
    return __spirv_ConvertFToU_Rulong_sat_rte( _T );
}

ulong OVERLOADABLE convert_ulong_sat_rtp(half _T) {
    return __spirv_ConvertFToU_Rulong_sat_rtp( _T );
}

ulong OVERLOADABLE convert_ulong_sat_rtn(half _T) {
    return __spirv_ConvertFToU_Rulong_sat_rtn( _T );
}

ulong OVERLOADABLE convert_ulong_sat_rtz(half _T) {
    return __spirv_ConvertFToU_Rulong_sat_rtz( _T );
}

char OVERLOADABLE convert_char(half _T) {
    return __spirv_ConvertFToS_Rchar( _T );
}

char OVERLOADABLE convert_char_rte(half _T) {
    return __spirv_ConvertFToS_Rchar_rte( _T );
}

char OVERLOADABLE convert_char_rtp(half _T) {
    return __spirv_ConvertFToS_Rchar_rtp( _T );
}

char OVERLOADABLE convert_char_rtn(half _T) {
    return __spirv_ConvertFToS_Rchar_rtn( _T );
}

char OVERLOADABLE convert_char_rtz(half _T) {
    return __spirv_ConvertFToS_Rchar_rtz( _T );
}

char OVERLOADABLE convert_char_sat(half _T) {
    return __spirv_ConvertFToS_Rchar_sat( _T );
}

char OVERLOADABLE convert_char_sat_rte(half _T) {
    return __spirv_ConvertFToS_Rchar_sat_rte( _T );
}

char OVERLOADABLE convert_char_sat_rtp(half _T) {
    return __spirv_ConvertFToS_Rchar_sat_rtp( _T );
}

char OVERLOADABLE convert_char_sat_rtn(half _T) {
    return __spirv_ConvertFToS_Rchar_sat_rtn( _T );
}

char OVERLOADABLE convert_char_sat_rtz(half _T) {
    return __spirv_ConvertFToS_Rchar_sat_rtz( _T );
}

short OVERLOADABLE convert_short(half _T) {
    return __spirv_ConvertFToS_Rshort( _T );
}

short OVERLOADABLE convert_short_rte(half _T) {
    return __spirv_ConvertFToS_Rshort_rte( _T );
}

short OVERLOADABLE convert_short_rtp(half _T) {
    return __spirv_ConvertFToS_Rshort_rtp( _T );
}

short OVERLOADABLE convert_short_rtn(half _T) {
    return __spirv_ConvertFToS_Rshort_rtn( _T );
}

short OVERLOADABLE convert_short_rtz(half _T) {
    return __spirv_ConvertFToS_Rshort_rtz( _T );
}

short OVERLOADABLE convert_short_sat(half _T) {
    return __spirv_ConvertFToS_Rshort_sat( _T );
}

short OVERLOADABLE convert_short_sat_rte(half _T) {
    return __spirv_ConvertFToS_Rshort_sat_rte( _T );
}

short OVERLOADABLE convert_short_sat_rtp(half _T) {
    return __spirv_ConvertFToS_Rshort_sat_rtp( _T );
}

short OVERLOADABLE convert_short_sat_rtn(half _T) {
    return __spirv_ConvertFToS_Rshort_sat_rtn( _T );
}

short OVERLOADABLE convert_short_sat_rtz(half _T) {
    return __spirv_ConvertFToS_Rshort_sat_rtz( _T );
}

int OVERLOADABLE convert_int(half _T) {
    return __spirv_ConvertFToS_Rint( _T );
}

int OVERLOADABLE convert_int_rte(half _T) {
    return __spirv_ConvertFToS_Rint_rte( _T );
}

int OVERLOADABLE convert_int_rtp(half _T) {
    return __spirv_ConvertFToS_Rint_rtp( _T );
}

int OVERLOADABLE convert_int_rtn(half _T) {
    return __spirv_ConvertFToS_Rint_rtn( _T );
}

int OVERLOADABLE convert_int_rtz(half _T) {
    return __spirv_ConvertFToS_Rint_rtz( _T );
}

int OVERLOADABLE convert_int_sat(half _T) {
    return __spirv_ConvertFToS_Rint_sat( _T );
}

int OVERLOADABLE convert_int_sat_rte(half _T) {
    return __spirv_ConvertFToS_Rint_sat_rte( _T );
}

int OVERLOADABLE convert_int_sat_rtp(half _T) {
    return __spirv_ConvertFToS_Rint_sat_rtp( _T );
}

int OVERLOADABLE convert_int_sat_rtn(half _T) {
    return __spirv_ConvertFToS_Rint_sat_rtn( _T );
}

int OVERLOADABLE convert_int_sat_rtz(half _T) {
    return __spirv_ConvertFToS_Rint_sat_rtz( _T );
}

long OVERLOADABLE convert_long(half _T) {
    return __spirv_ConvertFToS_Rlong( _T );
}

long OVERLOADABLE convert_long_rte(half _T) {
    return __spirv_ConvertFToS_Rlong_rte( _T );
}

long OVERLOADABLE convert_long_rtp(half _T) {
    return __spirv_ConvertFToS_Rlong_rtp( _T );
}

long OVERLOADABLE convert_long_rtn(half _T) {
    return __spirv_ConvertFToS_Rlong_rtn( _T );
}

long OVERLOADABLE convert_long_rtz(half _T) {
    return __spirv_ConvertFToS_Rlong_rtz( _T );
}

long OVERLOADABLE convert_long_sat(half _T) {
    return __spirv_ConvertFToS_Rlong_sat( _T );
}

long OVERLOADABLE convert_long_sat_rte(half _T) {
    return __spirv_ConvertFToS_Rlong_sat_rte( _T );
}

long OVERLOADABLE convert_long_sat_rtp(half _T) {
    return __spirv_ConvertFToS_Rlong_sat_rtp( _T );
}

long OVERLOADABLE convert_long_sat_rtn(half _T) {
    return __spirv_ConvertFToS_Rlong_sat_rtn( _T );
}

long OVERLOADABLE convert_long_sat_rtz(half _T) {
    return __spirv_ConvertFToS_Rlong_sat_rtz( _T );
}

float OVERLOADABLE convert_float(half _T) {
    return __spirv_FConvert_Rfloat( _T );
}

float OVERLOADABLE convert_float_rte(half _T) {
    return __spirv_FConvert_Rfloat_rte( _T );
}

float OVERLOADABLE convert_float_rtp(half _T) {
    return __spirv_FConvert_Rfloat_rtp( _T );
}

float OVERLOADABLE convert_float_rtn(half _T) {
    return __spirv_FConvert_Rfloat_rtn( _T );
}

float OVERLOADABLE convert_float_rtz(half _T) {
    return __spirv_FConvert_Rfloat_rtz( _T );
}

half OVERLOADABLE convert_half(uchar _T) {
    return __spirv_ConvertUToF_Rhalf( _T );
}

half OVERLOADABLE convert_half(ushort _T) {
    return __spirv_ConvertUToF_Rhalf( _T );
}

half OVERLOADABLE convert_half(uint _T) {
    return __spirv_ConvertUToF_Rhalf( _T );
}

half OVERLOADABLE convert_half(ulong _T) {
    return __spirv_ConvertUToF_Rhalf( _T );
}

half OVERLOADABLE convert_half(char _T) {
    return __spirv_ConvertSToF_Rhalf( _T );
}

half OVERLOADABLE convert_half(short _T) {
    return __spirv_ConvertSToF_Rhalf( _T );
}

half OVERLOADABLE convert_half(int _T) {
    return __spirv_ConvertSToF_Rhalf( _T );
}

half OVERLOADABLE convert_half(long _T) {
    return __spirv_ConvertSToF_Rhalf( _T );
}

half OVERLOADABLE convert_half(float _T) {
    return __spirv_FConvert_Rhalf( _T );
}

half OVERLOADABLE convert_half(half _T) {
    return __spirv_FConvert_Rhalf( _T );
}

half OVERLOADABLE convert_half_rte(uchar _T) {
    return __spirv_ConvertUToF_Rhalf_rte( _T );
}

half OVERLOADABLE convert_half_rte(ushort _T) {
    return __spirv_ConvertUToF_Rhalf_rte( _T );
}

half OVERLOADABLE convert_half_rte(uint _T) {
    return __spirv_ConvertUToF_Rhalf_rte( _T );
}

half OVERLOADABLE convert_half_rte(ulong _T) {
    return __spirv_ConvertUToF_Rhalf_rte( _T );
}

half OVERLOADABLE convert_half_rte(char _T) {
    return __spirv_ConvertSToF_Rhalf_rte( _T );
}

half OVERLOADABLE convert_half_rte(short _T) {
    return __spirv_ConvertSToF_Rhalf_rte( _T );
}

half OVERLOADABLE convert_half_rte(int _T) {
    return __spirv_ConvertSToF_Rhalf_rte( _T );
}

half OVERLOADABLE convert_half_rte(long _T) {
    return __spirv_ConvertSToF_Rhalf_rte( _T );
}

half OVERLOADABLE convert_half_rte(float _T) {
    return __spirv_FConvert_Rhalf_rte( _T );
}

half OVERLOADABLE convert_half_rte(half _T) {
    return __spirv_FConvert_Rhalf_rte( _T );
}

half OVERLOADABLE convert_half_rtp(uchar _T) {
    return __spirv_ConvertUToF_Rhalf_rtp( _T );
}

half OVERLOADABLE convert_half_rtp(ushort _T) {
    return __spirv_ConvertUToF_Rhalf_rtp( _T );
}

half OVERLOADABLE convert_half_rtp(uint _T) {
    return __spirv_ConvertUToF_Rhalf_rtp( _T );
}

half OVERLOADABLE convert_half_rtp(ulong _T) {
    return __spirv_ConvertUToF_Rhalf_rtp( _T );
}

half OVERLOADABLE convert_half_rtp(char _T) {
    return __spirv_ConvertSToF_Rhalf_rtp( _T );
}

half OVERLOADABLE convert_half_rtp(short _T) {
    return __spirv_ConvertSToF_Rhalf_rtp( _T );
}

half OVERLOADABLE convert_half_rtp(int _T) {
    return __spirv_ConvertSToF_Rhalf_rtp( _T );
}

half OVERLOADABLE convert_half_rtp(long _T) {
    return __spirv_ConvertSToF_Rhalf_rtp( _T );
}

half OVERLOADABLE convert_half_rtp(float _T) {
    return __spirv_FConvert_Rhalf_rtp( _T );
}

half OVERLOADABLE convert_half_rtp(half _T) {
    return __spirv_FConvert_Rhalf_rtp( _T );
}

half OVERLOADABLE convert_half_rtn(uchar _T) {
    return __spirv_ConvertUToF_Rhalf_rtn( _T );
}

half OVERLOADABLE convert_half_rtn(ushort _T) {
    return __spirv_ConvertUToF_Rhalf_rtn( _T );
}

half OVERLOADABLE convert_half_rtn(uint _T) {
    return __spirv_ConvertUToF_Rhalf_rtn( _T );
}

half OVERLOADABLE convert_half_rtn(ulong _T) {
    return __spirv_ConvertUToF_Rhalf_rtn( _T );
}

half OVERLOADABLE convert_half_rtn(char _T) {
    return __spirv_ConvertSToF_Rhalf_rtn( _T );
}

half OVERLOADABLE convert_half_rtn(short _T) {
    return __spirv_ConvertSToF_Rhalf_rtn( _T );
}

half OVERLOADABLE convert_half_rtn(int _T) {
    return __spirv_ConvertSToF_Rhalf_rtn( _T );
}

half OVERLOADABLE convert_half_rtn(long _T) {
    return __spirv_ConvertSToF_Rhalf_rtn( _T );
}

half OVERLOADABLE convert_half_rtn(float _T) {
    return __spirv_FConvert_Rhalf_rtn( _T );
}

half OVERLOADABLE convert_half_rtn(half _T) {
    return __spirv_FConvert_Rhalf_rtn( _T );
}

half OVERLOADABLE convert_half_rtz(uchar _T) {
    return __spirv_ConvertUToF_Rhalf_rtz( _T );
}

half OVERLOADABLE convert_half_rtz(ushort _T) {
    return __spirv_ConvertUToF_Rhalf_rtz( _T );
}

half OVERLOADABLE convert_half_rtz(uint _T) {
    return __spirv_ConvertUToF_Rhalf_rtz( _T );
}

half OVERLOADABLE convert_half_rtz(ulong _T) {
    return __spirv_ConvertUToF_Rhalf_rtz( _T );
}

half OVERLOADABLE convert_half_rtz(char _T) {
    return __spirv_ConvertSToF_Rhalf_rtz( _T );
}

half OVERLOADABLE convert_half_rtz(short _T) {
    return __spirv_ConvertSToF_Rhalf_rtz( _T );
}

half OVERLOADABLE convert_half_rtz(int _T) {
    return __spirv_ConvertSToF_Rhalf_rtz( _T );
}

half OVERLOADABLE convert_half_rtz(long _T) {
    return __spirv_ConvertSToF_Rhalf_rtz( _T );
}

half OVERLOADABLE convert_half_rtz(float _T) {
    return __spirv_FConvert_Rhalf_rtz( _T );
}

half OVERLOADABLE convert_half_rtz(half _T) {
    return __spirv_FConvert_Rhalf_rtz( _T );
}

GENERATE_CONVERSIONS_FUNCTIONS( convert_char,   char,   half )
GENERATE_CONVERSIONS_FUNCTIONS( convert_short,  short,  half )
GENERATE_CONVERSIONS_FUNCTIONS( convert_int,    int,    half )
GENERATE_CONVERSIONS_FUNCTIONS( convert_long,   long,   half )
GENERATE_CONVERSIONS_FUNCTIONS( convert_uchar,  uchar,  half )
GENERATE_CONVERSIONS_FUNCTIONS( convert_ushort, ushort, half )
GENERATE_CONVERSIONS_FUNCTIONS( convert_uint,   uint,   half )
GENERATE_CONVERSIONS_FUNCTIONS( convert_ulong,  ulong,  half )
GENERATE_CONVERSIONS_FUNCTIONS( convert_float,  float,  half )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_char,   char,   half )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_short,  short,  half )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_int,    int,    half )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_long,   long,   half )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_uchar,  uchar,  half )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_ushort, ushort, half )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_uint,   uint,   half )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_ulong,  ulong,  half )

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES( convert_half, half )
GENERATE_CONVERSIONS_FUNCTIONS( convert_half, half, half )

#endif // defined(cl_khr_fp16)

#if defined(cl_khr_fp64)

uchar OVERLOADABLE convert_uchar(double _T) {
    return __spirv_ConvertFToU_Ruchar( _T );
}

uchar OVERLOADABLE convert_uchar_rte(double _T) {
    return __spirv_ConvertFToU_Ruchar_rte( _T );
}

uchar OVERLOADABLE convert_uchar_rtp(double _T) {
    return __spirv_ConvertFToU_Ruchar_rtp( _T );
}

uchar OVERLOADABLE convert_uchar_rtn(double _T) {
    return __spirv_ConvertFToU_Ruchar_rtn( _T );
}

uchar OVERLOADABLE convert_uchar_rtz(double _T) {
    return __spirv_ConvertFToU_Ruchar_rtz( _T );
}

uchar OVERLOADABLE convert_uchar_sat(double _T) {
    return __spirv_ConvertFToU_Ruchar_sat( _T );
}

uchar OVERLOADABLE convert_uchar_sat_rte(double _T) {
    return __spirv_ConvertFToU_Ruchar_sat_rte( _T );
}

uchar OVERLOADABLE convert_uchar_sat_rtp(double _T) {
    return __spirv_ConvertFToU_Ruchar_sat_rtp( _T );
}

uchar OVERLOADABLE convert_uchar_sat_rtn(double _T) {
    return __spirv_ConvertFToU_Ruchar_sat_rtn( _T );
}

uchar OVERLOADABLE convert_uchar_sat_rtz(double _T) {
    return __spirv_ConvertFToU_Ruchar_sat_rtz( _T );
}

ushort OVERLOADABLE convert_ushort(double _T) {
    return __spirv_ConvertFToU_Rushort( _T );
}

ushort OVERLOADABLE convert_ushort_rte(double _T) {
    return __spirv_ConvertFToU_Rushort_rte( _T );
}

ushort OVERLOADABLE convert_ushort_rtp(double _T) {
    return __spirv_ConvertFToU_Rushort_rtp( _T );
}

ushort OVERLOADABLE convert_ushort_rtn(double _T) {
    return __spirv_ConvertFToU_Rushort_rtn( _T );
}

ushort OVERLOADABLE convert_ushort_rtz(double _T) {
    return __spirv_ConvertFToU_Rushort_rtz( _T );
}

ushort OVERLOADABLE convert_ushort_sat(double _T) {
    return __spirv_ConvertFToU_Rushort_sat( _T );
}

ushort OVERLOADABLE convert_ushort_sat_rte(double _T) {
    return __spirv_ConvertFToU_Rushort_sat_rte( _T );
}

ushort OVERLOADABLE convert_ushort_sat_rtp(double _T) {
    return __spirv_ConvertFToU_Rushort_sat_rtp( _T );
}

ushort OVERLOADABLE convert_ushort_sat_rtn(double _T) {
    return __spirv_ConvertFToU_Rushort_sat_rtn( _T );
}

ushort OVERLOADABLE convert_ushort_sat_rtz(double _T) {
    return __spirv_ConvertFToU_Rushort_sat_rtz( _T );
}

uint OVERLOADABLE convert_uint(double _T) {
    return __spirv_ConvertFToU_Ruint( _T );
}

uint OVERLOADABLE convert_uint_rte(double _T) {
    return __spirv_ConvertFToU_Ruint_rte( _T );
}

uint OVERLOADABLE convert_uint_rtp(double _T) {
    return __spirv_ConvertFToU_Ruint_rtp( _T );
}

uint OVERLOADABLE convert_uint_rtn(double _T) {
    return __spirv_ConvertFToU_Ruint_rtn( _T );
}

uint OVERLOADABLE convert_uint_rtz(double _T) {
    return __spirv_ConvertFToU_Ruint_rtz( _T );
}

uint OVERLOADABLE convert_uint_sat(double _T) {
    return __spirv_ConvertFToU_Ruint_sat( _T );
}

uint OVERLOADABLE convert_uint_sat_rte(double _T) {
    return __spirv_ConvertFToU_Ruint_sat_rte( _T );
}

uint OVERLOADABLE convert_uint_sat_rtp(double _T) {
    return __spirv_ConvertFToU_Ruint_sat_rtp( _T );
}

uint OVERLOADABLE convert_uint_sat_rtn(double _T) {
    return __spirv_ConvertFToU_Ruint_sat_rtn( _T );
}

uint OVERLOADABLE convert_uint_sat_rtz(double _T) {
    return __spirv_ConvertFToU_Ruint_sat_rtz( _T );
}

ulong OVERLOADABLE convert_ulong(double _T) {
    return __spirv_ConvertFToU_Rulong( _T );
}

ulong OVERLOADABLE convert_ulong_rte(double _T) {
    return __spirv_ConvertFToU_Rulong_rte( _T );
}

ulong OVERLOADABLE convert_ulong_rtp(double _T) {
    return __spirv_ConvertFToU_Rulong_rtp( _T );
}

ulong OVERLOADABLE convert_ulong_rtn(double _T) {
    return __spirv_ConvertFToU_Rulong_rtn( _T );
}

ulong OVERLOADABLE convert_ulong_rtz(double _T) {
    return __spirv_ConvertFToU_Rulong_rtz( _T );
}

ulong OVERLOADABLE convert_ulong_sat(double _T) {
    return __spirv_ConvertFToU_Rulong_sat( _T );
}

ulong OVERLOADABLE convert_ulong_sat_rte(double _T) {
    return __spirv_ConvertFToU_Rulong_sat_rte( _T );
}

ulong OVERLOADABLE convert_ulong_sat_rtp(double _T) {
    return __spirv_ConvertFToU_Rulong_sat_rtp( _T );
}

ulong OVERLOADABLE convert_ulong_sat_rtn(double _T) {
    return __spirv_ConvertFToU_Rulong_sat_rtn( _T );
}

ulong OVERLOADABLE convert_ulong_sat_rtz(double _T) {
    return __spirv_ConvertFToU_Rulong_sat_rtz( _T );
}

char OVERLOADABLE convert_char(double _T) {
    return __spirv_ConvertFToS_Rchar( _T );
}

char OVERLOADABLE convert_char_rte(double _T) {
    return __spirv_ConvertFToS_Rchar_rte( _T );
}

char OVERLOADABLE convert_char_rtp(double _T) {
    return __spirv_ConvertFToS_Rchar_rtp( _T );
}

char OVERLOADABLE convert_char_rtn(double _T) {
    return __spirv_ConvertFToS_Rchar_rtn( _T );
}

char OVERLOADABLE convert_char_rtz(double _T) {
    return __spirv_ConvertFToS_Rchar_rtz( _T );
}

char OVERLOADABLE convert_char_sat(double _T) {
    return __spirv_ConvertFToS_Rchar_sat( _T );
}

char OVERLOADABLE convert_char_sat_rte(double _T) {
    return __spirv_ConvertFToS_Rchar_sat_rte( _T );
}

char OVERLOADABLE convert_char_sat_rtp(double _T) {
    return __spirv_ConvertFToS_Rchar_sat_rtp( _T );
}

char OVERLOADABLE convert_char_sat_rtn(double _T) {
    return __spirv_ConvertFToS_Rchar_sat_rtn( _T );
}

char OVERLOADABLE convert_char_sat_rtz(double _T) {
    return __spirv_ConvertFToS_Rchar_sat_rtz( _T );
}

short OVERLOADABLE convert_short(double _T) {
    return __spirv_ConvertFToS_Rshort( _T );
}

short OVERLOADABLE convert_short_rte(double _T) {
    return __spirv_ConvertFToS_Rshort_rte( _T );
}

short OVERLOADABLE convert_short_rtp(double _T) {
    return __spirv_ConvertFToS_Rshort_rtp( _T );
}

short OVERLOADABLE convert_short_rtn(double _T) {
    return __spirv_ConvertFToS_Rshort_rtn( _T );
}

short OVERLOADABLE convert_short_rtz(double _T) {
    return __spirv_ConvertFToS_Rshort_rtz( _T );
}

short OVERLOADABLE convert_short_sat(double _T) {
    return __spirv_ConvertFToS_Rshort_sat( _T );
}

short OVERLOADABLE convert_short_sat_rte(double _T) {
    return __spirv_ConvertFToS_Rshort_sat_rte( _T );
}

short OVERLOADABLE convert_short_sat_rtp(double _T) {
    return __spirv_ConvertFToS_Rshort_sat_rtp( _T );
}

short OVERLOADABLE convert_short_sat_rtn(double _T) {
    return __spirv_ConvertFToS_Rshort_sat_rtn( _T );
}

short OVERLOADABLE convert_short_sat_rtz(double _T) {
    return __spirv_ConvertFToS_Rshort_sat_rtz( _T );
}

int OVERLOADABLE convert_int(double _T) {
    return __spirv_ConvertFToS_Rint( _T );
}

int OVERLOADABLE convert_int_rte(double _T) {
    return __spirv_ConvertFToS_Rint_rte( _T );
}

int OVERLOADABLE convert_int_rtp(double _T) {
    return __spirv_ConvertFToS_Rint_rtp( _T );
}

int OVERLOADABLE convert_int_rtn(double _T) {
    return __spirv_ConvertFToS_Rint_rtn( _T );
}

int OVERLOADABLE convert_int_rtz(double _T) {
    return __spirv_ConvertFToS_Rint_rtz( _T );
}

int OVERLOADABLE convert_int_sat(double _T) {
    return __spirv_ConvertFToS_Rint_sat( _T );
}

int OVERLOADABLE convert_int_sat_rte(double _T) {
    return __spirv_ConvertFToS_Rint_sat_rte( _T );
}

int OVERLOADABLE convert_int_sat_rtp(double _T) {
    return __spirv_ConvertFToS_Rint_sat_rtp( _T );
}

int OVERLOADABLE convert_int_sat_rtn(double _T) {
    return __spirv_ConvertFToS_Rint_sat_rtn( _T );
}

int OVERLOADABLE convert_int_sat_rtz(double _T) {
    return __spirv_ConvertFToS_Rint_sat_rtz( _T );
}

long OVERLOADABLE convert_long(double _T) {
    return __spirv_ConvertFToS_Rlong( _T );
}

long OVERLOADABLE convert_long_rte(double _T) {
    return __spirv_ConvertFToS_Rlong_rte( _T );
}

long OVERLOADABLE convert_long_rtp(double _T) {
    return __spirv_ConvertFToS_Rlong_rtp( _T );
}

long OVERLOADABLE convert_long_rtn(double _T) {
    return __spirv_ConvertFToS_Rlong_rtn( _T );
}

long OVERLOADABLE convert_long_rtz(double _T) {
    return __spirv_ConvertFToS_Rlong_rtz( _T );
}

long OVERLOADABLE convert_long_sat(double _T) {
    return __spirv_ConvertFToS_Rlong_sat( _T );
}

long OVERLOADABLE convert_long_sat_rte(double _T) {
    return __spirv_ConvertFToS_Rlong_sat_rte( _T );
}

long OVERLOADABLE convert_long_sat_rtp(double _T) {
    return __spirv_ConvertFToS_Rlong_sat_rtp( _T );
}

long OVERLOADABLE convert_long_sat_rtn(double _T) {
    return __spirv_ConvertFToS_Rlong_sat_rtn( _T );
}

long OVERLOADABLE convert_long_sat_rtz(double _T) {
    return __spirv_ConvertFToS_Rlong_sat_rtz( _T );
}

float OVERLOADABLE convert_float(double _T) {
    return __spirv_FConvert_Rfloat( _T );
}

float OVERLOADABLE convert_float_rte(double _T) {
    return __spirv_FConvert_Rfloat_rte( _T );
}

INLINE float OVERLOADABLE convert_float_rtp(double _T)
{
    return __spirv_FConvert_Rfloat_rtp( _T );
}

INLINE float OVERLOADABLE convert_float_rtn(double _T)
{
    return __spirv_FConvert_Rfloat_rtn( _T );
}

INLINE float OVERLOADABLE convert_float_rtz(double _T)
{
    return __spirv_FConvert_Rfloat_rtz( _T );
}

GENERATE_CONVERSIONS_FUNCTIONS( convert_char,   char,   double )
GENERATE_CONVERSIONS_FUNCTIONS( convert_short,  short,  double )
GENERATE_CONVERSIONS_FUNCTIONS( convert_int,    int,    double )
GENERATE_CONVERSIONS_FUNCTIONS( convert_long,   long,   double )
GENERATE_CONVERSIONS_FUNCTIONS( convert_uchar,  uchar,  double )
GENERATE_CONVERSIONS_FUNCTIONS( convert_ushort, ushort, double )
GENERATE_CONVERSIONS_FUNCTIONS( convert_uint,   uint,   double )
GENERATE_CONVERSIONS_FUNCTIONS( convert_ulong,  ulong,  double )
GENERATE_CONVERSIONS_FUNCTIONS( convert_float,  float,  double )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_char,   char,   double )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_short,  short,  double )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_int,    int,    double )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_long,   long,   double )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_uchar,  uchar,  double )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_ushort, ushort, double )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_uint,   uint,   double )
GENERATE_CONVERSIONS_FUNCTIONS_SAT( convert_ulong,  ulong,  double )


double OVERLOADABLE convert_double(uchar _T) {
    return __spirv_ConvertUToF_Rdouble( _T );
}

double OVERLOADABLE convert_double(ushort _T) {
    return __spirv_ConvertUToF_Rdouble( _T );
}

double OVERLOADABLE convert_double(uint _T) {
    return __spirv_ConvertUToF_Rdouble( _T );
}

double OVERLOADABLE convert_double(ulong _T) {
    return __spirv_ConvertUToF_Rdouble( _T );
}

double OVERLOADABLE convert_double(char _T) {
    return __spirv_ConvertSToF_Rdouble( _T );
}

double OVERLOADABLE convert_double(short _T) {
    return __spirv_ConvertSToF_Rdouble( _T );
}

double OVERLOADABLE convert_double(int _T) {
    return __spirv_ConvertSToF_Rdouble( _T );
}

double OVERLOADABLE convert_double(long _T) {
    return __spirv_ConvertSToF_Rdouble( _T );
}

double OVERLOADABLE convert_double(float _T) {
    return __spirv_FConvert_Rdouble( _T );
}

double OVERLOADABLE convert_double(double _T) {
    return __spirv_FConvert_Rdouble( _T );
}

double OVERLOADABLE convert_double_rte(uchar _T) {
    return __spirv_ConvertUToF_Rdouble_rte( _T );
}

double OVERLOADABLE convert_double_rte(ushort _T) {
    return __spirv_ConvertUToF_Rdouble_rte( _T );
}

double OVERLOADABLE convert_double_rte(uint _T) {
    return __spirv_ConvertUToF_Rdouble_rte( _T );
}

double OVERLOADABLE convert_double_rte(ulong _T) {
    return __spirv_ConvertUToF_Rdouble_rte( _T );
}

double OVERLOADABLE convert_double_rte(char _T) {
    return __spirv_ConvertSToF_Rdouble_rte( _T );
}

double OVERLOADABLE convert_double_rte(short _T) {
    return __spirv_ConvertSToF_Rdouble_rte( _T );
}

double OVERLOADABLE convert_double_rte(int _T) {
    return __spirv_ConvertSToF_Rdouble_rte( _T );
}

double OVERLOADABLE convert_double_rte(long _T) {
    return __spirv_ConvertSToF_Rdouble_rte( _T );
}

double OVERLOADABLE convert_double_rte(float _T) {
    return __spirv_FConvert_Rdouble_rte( _T );
}

double OVERLOADABLE convert_double_rte(double _T) {
    return __spirv_FConvert_Rdouble_rte( _T );
}

double OVERLOADABLE convert_double_rtp(uchar _T) {
    return __spirv_ConvertUToF_Rdouble_rtp( _T );
}

double OVERLOADABLE convert_double_rtp(ushort _T) {
    return __spirv_ConvertUToF_Rdouble_rtp( _T );
}

double OVERLOADABLE convert_double_rtp(uint _T) {
    return __spirv_ConvertUToF_Rdouble_rtp( _T );
}

double OVERLOADABLE convert_double_rtp(char _T) {
    return __spirv_ConvertSToF_Rdouble_rtp( _T );
}

double OVERLOADABLE convert_double_rtp(short _T) {
    return __spirv_ConvertSToF_Rdouble_rtp( _T );
}

double OVERLOADABLE convert_double_rtp(int _T) {
    return __spirv_ConvertSToF_Rdouble_rtp( _T );
}

double OVERLOADABLE convert_double_rtp(float _T) {
    return __spirv_FConvert_Rdouble_rtp( _T );
}

double OVERLOADABLE convert_double_rtp(double _T) {
    return __spirv_FConvert_Rdouble_rtp( _T );
}

double OVERLOADABLE convert_double_rtn(uchar _T) {
    return __spirv_ConvertUToF_Rdouble_rtn( _T );
}

double OVERLOADABLE convert_double_rtn(ushort _T) {
    return __spirv_ConvertUToF_Rdouble_rtn( _T );
}

double OVERLOADABLE convert_double_rtn(uint _T) {
    return __spirv_ConvertUToF_Rdouble_rtn( _T );
}

double OVERLOADABLE convert_double_rtn(char _T) {
    return __spirv_ConvertSToF_Rdouble_rtn( _T );
}

double OVERLOADABLE convert_double_rtn(short _T) {
    return __spirv_ConvertSToF_Rdouble_rtn( _T );
}

double OVERLOADABLE convert_double_rtn(int _T) {
    return __spirv_ConvertSToF_Rdouble_rtn( _T );
}

double OVERLOADABLE convert_double_rtn(float _T) {
    return __spirv_FConvert_Rdouble_rtn( _T );
}

double OVERLOADABLE convert_double_rtn(double _T) {
    return __spirv_FConvert_Rdouble_rtn( _T );
}

double OVERLOADABLE convert_double_rtz(uchar _T) {
    return __spirv_ConvertUToF_Rdouble_rtz( _T );
}

double OVERLOADABLE convert_double_rtz(ushort _T) {
    return __spirv_ConvertUToF_Rdouble_rtz( _T );
}

double OVERLOADABLE convert_double_rtz(uint _T) {
    return __spirv_ConvertUToF_Rdouble_rtz( _T );
}

double OVERLOADABLE convert_double_rtz(char _T) {
    return __spirv_ConvertSToF_Rdouble_rtz( _T );
}

double OVERLOADABLE convert_double_rtz(short _T) {
    return __spirv_ConvertSToF_Rdouble_rtz( _T );
}

double OVERLOADABLE convert_double_rtz(int _T) {
    return __spirv_ConvertSToF_Rdouble_rtz( _T );
}

double OVERLOADABLE convert_double_rtz(float _T) {
    return __spirv_FConvert_Rdouble_rtz( _T );
}

double OVERLOADABLE convert_double_rtz(double _T) {
    return __spirv_FConvert_Rdouble_rtz( _T );
}

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES( convert_double, double )
GENERATE_CONVERSIONS_FUNCTIONS( convert_double, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16) && defined(cl_khr_fp64)

double OVERLOADABLE convert_double(half _T) {
    return __spirv_FConvert_Rdouble( _T );
}

double OVERLOADABLE convert_double_rte(half _T) {
    return __spirv_FConvert_Rdouble_rte( _T );
}

double OVERLOADABLE convert_double_rtp(half _T) {
    return __spirv_FConvert_Rdouble_rtp( _T );
}

double OVERLOADABLE convert_double_rtn(half _T) {
    return __spirv_FConvert_Rdouble_rtn( _T );
}

double OVERLOADABLE convert_double_rtz(half _T) {
    return __spirv_FConvert_Rdouble_rtz( _T );
}

GENERATE_CONVERSIONS_FUNCTIONS( convert_double, double, half )

half OVERLOADABLE convert_half(double _T) {
    return __spirv_FConvert_Rhalf( _T );
}

half OVERLOADABLE convert_half_rte(double _T) {
    return __spirv_FConvert_Rhalf_rte( _T );
}

half OVERLOADABLE convert_half_rtp(double _T) {
    return __spirv_FConvert_Rhalf_rtp( _T );
}

half OVERLOADABLE convert_half_rtn(double _T) {
    return __spirv_FConvert_Rhalf_rtn( _T );
}

half OVERLOADABLE convert_half_rtz(double _T) {
    return __spirv_FConvert_Rhalf_rtz( _T );
}

GENERATE_CONVERSIONS_FUNCTIONS( convert_half, half, double )

#endif

// FP4 / INT4 conversion functions for Intel validation

#define SHFLIDX_CONVERT_SINGLE(                                                                                    \
    OUTTYPE, INTYPE, VECLENGTH, PACKEDLENGTH, LUT, FPTYPE, FPBITLEN, I4ORF4, OUTTYPEPACKED)                        \
    OUTTYPE##VECLENGTH                                                                                             \
        intel_convert_as_##I4ORF4##VECLENGTH##_##FPTYPE##VECLENGTH##_as_##OUTTYPE##VECLENGTH(                      \
            INTYPE##VECLENGTH source)                                                                              \
    {                                                                                                              \
        return __builtin_IB_shfl_idx4_to_fp##FPBITLEN##_##VECLENGTH(                                               \
            __builtin_IB_shfl_idx4_lut(LUT), as_char##VECLENGTH(source));                                          \
    }                                                                                                              \
    OUTTYPEPACKED##VECLENGTH                                                                                       \
        intel_convert_as_##I4ORF4##PACKEDLENGTH##_##FPTYPE##PACKEDLENGTH##_as_##OUTTYPEPACKED##VECLENGTH##_packed( \
            INTYPE##VECLENGTH source)                                                                              \
    {                                                                                                              \
        return __builtin_IB_shfl_idx4_to_fp##FPBITLEN##_##VECLENGTH##_packed(                                      \
            __builtin_IB_shfl_idx4_lut(LUT), as_char##VECLENGTH(source));                                          \
    }

#define SHFLIDX_CONVERT(OUTTYPE, INTYPE, LUT, FPTYPE, FPBITLEN, I4ORF4, OUTTYPEPACKED)      \
    OUTTYPE intel_convert_as_##I4ORF4##_##FPTYPE##_as_##OUTTYPE(INTYPE source)              \
    {                                                                                       \
        return __builtin_IB_shfl_idx4_to_fp##FPBITLEN(                                      \
            __builtin_IB_shfl_idx4_lut(LUT), source);                                       \
    }                                                                                       \
    OUTTYPEPACKED intel_convert_as_##I4ORF4##2_##FPTYPE##2_as_##OUTTYPEPACKED##_packed(     \
        INTYPE source)                                                                      \
    {                                                                                       \
        return __builtin_IB_shfl_idx4_to_fp##FPBITLEN##_packed(                             \
            __builtin_IB_shfl_idx4_lut(LUT), source);                                       \
    }                                                                                       \
    SHFLIDX_CONVERT_SINGLE(                                                                 \
        OUTTYPE, INTYPE, 2, 4, LUT, FPTYPE, FPBITLEN, I4ORF4, OUTTYPEPACKED)                \
    SHFLIDX_CONVERT_SINGLE(                                                                 \
        OUTTYPE, INTYPE, 4, 8, LUT, FPTYPE, FPBITLEN, I4ORF4, OUTTYPEPACKED)                \
    SHFLIDX_CONVERT_SINGLE(                                                                 \
        OUTTYPE, INTYPE, 8, 16, LUT, FPTYPE, FPBITLEN, I4ORF4, OUTTYPEPACKED)               \
    SHFLIDX_CONVERT_SINGLE(                                                                 \
        OUTTYPE, INTYPE, 16, 32, LUT, FPTYPE, FPBITLEN, I4ORF4, OUTTYPEPACKED)              \
    OUTTYPE##3 intel_convert_as_##I4ORF4##3_##FPTYPE##3_as_##OUTTYPE##3(INTYPE##3 source)   \
    {                                                                                       \
        INTYPE##4 s  = (INTYPE##4)(source.x, source.y, source.z, 0);                        \
        OUTTYPE##4 r = __builtin_IB_shfl_idx4_to_fp##FPBITLEN##_##4(                        \
            __builtin_IB_shfl_idx4_lut(LUT), as_char4(s));                                  \
        return r.xyz;                                                                       \
    }                                                                                       \
    OUTTYPEPACKED##3 intel_convert_as_##I4ORF4##6_##FPTYPE##6_as_##OUTTYPEPACKED##3_packed( \
        INTYPE##3 source)                                                                   \
    {                                                                                       \
        INTYPE##4 s = (INTYPE##4)(source.x, source.y, source.z, 0);                         \
        OUTTYPEPACKED##4 r =                                                                \
            __builtin_IB_shfl_idx4_to_fp##FPBITLEN##_##4_packed(                            \
                __builtin_IB_shfl_idx4_lut(LUT), as_char4(s));                              \
        return r.xyz;                                                                       \
    }

#define SHFLIDX_CONVERT_AS_ITSELF_SINGLE(                                                                          \
    OUTTYPE, INTYPE, VECLENGTH, PACKEDLENGTH, LUT, FPTYPE, FPBITLEN, I4ORF4, OUTTYPEPACKED)                        \
    OUTTYPE##VECLENGTH                                                                                             \
        intel_convert_as_##I4ORF4##VECLENGTH##_##FPTYPE##VECLENGTH(                                                \
            INTYPE##VECLENGTH source)                                                                              \
    {                                                                                                              \
        return as_##OUTTYPE##VECLENGTH(                                                                            \
            __builtin_IB_shfl_idx4_to_fp##FPBITLEN##_##VECLENGTH(                                                  \
                __builtin_IB_shfl_idx4_lut(LUT), as_char##VECLENGTH(source)));                                     \
    }                                                                                                              \
    OUTTYPEPACKED##VECLENGTH                                                                                       \
        intel_convert_as_##I4ORF4##PACKEDLENGTH##_##FPTYPE##PACKEDLENGTH##_as_##OUTTYPEPACKED##VECLENGTH##_packed( \
            INTYPE##VECLENGTH source)                                                                              \
    {                                                                                                              \
        return __builtin_IB_shfl_idx4_to_fp##FPBITLEN##_##VECLENGTH##_packed( \
            __builtin_IB_shfl_idx4_lut(LUT), as_char##VECLENGTH(source));     \
    }

#define SHFLIDX_CONVERT_AS_ITSELF(OUTTYPE, INTYPE, LUT, FPTYPE, FPBITLEN, I4ORF4, OUTTYPEPACKED) \
    OUTTYPE intel_convert_as_##I4ORF4##_##FPTYPE(INTYPE source)                                  \
    {                                                                                            \
        return as_##OUTTYPE(__builtin_IB_shfl_idx4_to_fp##FPBITLEN(                              \
            __builtin_IB_shfl_idx4_lut(LUT), source));                                           \
    }                                                                                            \
    OUTTYPEPACKED intel_convert_as_##I4ORF4##2_##FPTYPE##2_as_##OUTTYPEPACKED##_packed(          \
        INTYPE source)                                                                           \
    {                                                                                            \
        return __builtin_IB_shfl_idx4_to_fp##FPBITLEN##_packed(                                  \
            __builtin_IB_shfl_idx4_lut(LUT), source);                                            \
    }                                                                                            \
    SHFLIDX_CONVERT_AS_ITSELF_SINGLE(                                                            \
        OUTTYPE, INTYPE, 2, 4, LUT, FPTYPE, FPBITLEN, I4ORF4, OUTTYPEPACKED)                     \
    SHFLIDX_CONVERT_AS_ITSELF_SINGLE(                                                            \
        OUTTYPE, INTYPE, 4, 8, LUT, FPTYPE, FPBITLEN, I4ORF4, OUTTYPEPACKED)                     \
    SHFLIDX_CONVERT_AS_ITSELF_SINGLE(                                                            \
        OUTTYPE, INTYPE, 8, 16, LUT, FPTYPE, FPBITLEN, I4ORF4, OUTTYPEPACKED)                    \
    SHFLIDX_CONVERT_AS_ITSELF_SINGLE(                                                            \
        OUTTYPE, INTYPE, 16, 32, LUT, FPTYPE, FPBITLEN, I4ORF4, OUTTYPEPACKED)                   \
    OUTTYPE##3 intel_convert_as_##I4ORF4##3_##FPTYPE##3(INTYPE##3 source)                        \
    {                                                                                            \
        INTYPE##4 s  = (INTYPE##4)(source.x, source.y, source.z, 0);                             \
        OUTTYPE##4 r =                                                                           \
            as_##OUTTYPE##4(__builtin_IB_shfl_idx4_to_fp##FPBITLEN##_##4(                        \
                __builtin_IB_shfl_idx4_lut(LUT), as_char4(s)));                                  \
        return r.xyz;                                                                            \
    }                                                                                            \
    OUTTYPEPACKED##3 intel_convert_as_##I4ORF4##6_##FPTYPE##6_as_##OUTTYPEPACKED##3_packed(      \
        INTYPE##3 source)                                                                        \
    {                                                                                            \
        INTYPE##4 s = (INTYPE##4)(source.x, source.y, source.z, 0);                              \
        OUTTYPEPACKED##4 r =                                                                     \
            __builtin_IB_shfl_idx4_to_fp##FPBITLEN##_##4_packed(                                 \
                __builtin_IB_shfl_idx4_lut(LUT), as_char4(s));                                   \
        return r.xyz;                                                                            \
    }

// int4, e2m1 -> bfloat8
SHFLIDX_CONVERT(uchar,  char, LUT_int4_to_bfloat8, bfloat8, 8, i4,   ushort)
SHFLIDX_CONVERT(uchar, uchar, LUT_e2m1_to_bfloat8, bfloat8, 8, e2m1, ushort)

// int4, e2m1 -> Hfloat8:
SHFLIDX_CONVERT(uchar,  char, LUT_int4_to_hfloat8, hfloat8, 8, i4,   ushort)
SHFLIDX_CONVERT(uchar, uchar, LUT_e2m1_to_hfloat8, hfloat8, 8, e2m1, ushort)

// int4, e2m1 -> Bfloat16:
SHFLIDX_CONVERT(ushort,  char, LUT_int4_to_bfloat16, bfloat16, 16, i4,   uint)
SHFLIDX_CONVERT(ushort, uchar, LUT_e2m1_to_bfloat16, bfloat16, 16, e2m1, uint)

#ifdef cl_khr_fp16
// int4, e2m1 -> Half:
SHFLIDX_CONVERT_AS_ITSELF(half,  char, LUT_int4_to_hfloat16, half, 16, i4,   uint)
SHFLIDX_CONVERT_AS_ITSELF(half, uchar, LUT_e2m1_to_hfloat16, half, 16, e2m1, uint)
#endif // cl_khr_fp16
