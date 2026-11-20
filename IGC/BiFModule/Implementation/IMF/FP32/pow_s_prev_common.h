/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Types, tables and helpers shared by pow_s_prev.cl and pown_s_prev.cl.
// The definitions live in pow_s_prev.cl.

#ifndef __POW_S_PREV_COMMON_H__
#define __POW_S_PREV_COMMON_H__

typedef int VINT32;
typedef signed int VSINT32;
typedef unsigned int VUINT32;

typedef long VINT64;
typedef signed long VSINT64;
typedef unsigned long VUINT64;

typedef int int1_gen;
typedef long long1_gen;
typedef float float1_gen;
typedef float float1_ref;
typedef int int1_ref;

typedef unsigned int _iml_uint32_t;
typedef int _iml_int32_t;
typedef unsigned long _iml_uint64_t;

typedef union {
  uint hex;
  float fp;
} _iml_pow_sp_union_t;

#if defined(cl_khr_fp64)

typedef double double1_ref;

typedef struct tag_iml_dpdwords_t {
  _iml_uint32_t lo_dword;
  _iml_uint32_t hi_dword;
} _iml_pow_dpdwords_t;

typedef union {
  _iml_uint32_t hex[2];
  _iml_pow_dpdwords_t dwords;
  double fp;
} _iml_pow_dp_union_t;

typedef union {
  ulong hex;
  double fp;
} _lng_dbl_union_t;

#endif

typedef struct {
  VUINT32 sHiMask;
  VUINT32 sRSValue;
  VUINT32 NMINNORM;
  VUINT32 NMAXVAL;
  VUINT32 INF;

  VUINT32 LFR_TBL[65][3];
  VUINT32 LFR_I_CHK_WORK_SUB;
  VUINT32 LFR_I_CHK_WORK_CMP;
  VUINT32 S_MANT_MASK;
  VUINT32 S_ONE;
  VUINT32 LFR_I_INDEX_MASK;
  VUINT32 LFR_I_INDEX_ADD;
  VUINT32 S_HI10BITS_MASK;
  VUINT32 LFR_S_P4;
  VUINT32 LFR_S_P3;
  VUINT32 LFR_S_P2;
  VUINT32 I_BIAS;
  VUINT32 LFR_I_NZ_ADD;
  VUINT32 LFR_I_NZ_CMP;
  VUINT32 S_LOG2_HI;
  VUINT32 S_LOG2_LO;

  VUINT32 _sInvLn2;
  VUINT32 _sShifter;
  VUINT32 _sLn2hi;
  VUINT32 _sLn2lo;
  VUINT32 _sPC0;
  VUINT32 _sPC1;
  VUINT32 _sPC2;
  VUINT32 _sPC3;
  VUINT32 _sPC4;
  VUINT32 _sPC5;
  VUINT32 _iBias;
  VUINT32 _iAbsMask;
  VUINT32 _iDomainRange;
  VUINT32 _s2N_2;
  VUINT32 _sHuge;
  VUINT32 _sTiny;

} sPow_Table_Type;

typedef struct {
  uint sHiMask;
  uint sRSValue;
  uint NMINNORM;
  uint NMAXVAL;
  uint INF;

  uint LFR_TBL[65][3][1];
  uint LFR_I_CHK_WORK_SUB;
  uint LFR_I_CHK_WORK_CMP;
  uint S_MANT_MASK;
  uint S_ONE;
  uint LFR_I_INDEX_MASK;
  uint LFR_I_INDEX_ADD;
  uint S_HI10BITS_MASK;
  uint LFR_S_P4;
  uint LFR_S_P3;
  uint LFR_S_P2;
  uint I_BIAS;
  uint LFR_I_NZ_ADD;
  uint LFR_I_NZ_CMP;
  uint S_LOG2_HI;
  uint S_LOG2_LO;

  uint _sInvLn2;
  uint _sShifter;
  uint _sLn2hi;
  uint _sLn2lo;
  uint _sPC0;
  uint _sPC1;
  uint _sPC2;
  uint _sPC3;
  uint _sPC4;
  uint _sPC5;
  uint _iBias;
  uint _iAbsMask;
  uint _iDomainRange;
  uint _s2N_2;
  uint _sHuge;
  uint _sTiny;

} sRootn_Table_Type;

uint _castf32_u32(float a);
float _castu32_f32(uint a);

#if defined(cl_khr_fp64)
double _castu64_f64(ulong a);
ulong _castf64_u64(double a);
double _nearbyint(double a);
#endif

extern __constant sPow_Table_Type __ocl_svml_spow_data;
extern __constant VUINT32 _vmlsPowHATab[380];
extern __constant sRootn_Table_Type __ocl_svml_srootn_data;
extern __constant _iml_pow_sp_union_t _vmlsRootnHATab[380];

int _TestInt(float a);
int _TestIntI(int a);

#endif // __POW_S_PREV_COMMON_H__
