/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Declarations of the functions defined in IBiF_Intrinsics_Impl.cl.

#ifndef __IBIF_INTRINSICS_IMPL_H__
#define __IBIF_INTRINSICS_IMPL_H__

float OVERLOADABLE clamp(float x, float minval, float maxval);
float OVERLOADABLE max(float x, float y);
float OVERLOADABLE min(float x, float y);
uchar OVERLOADABLE abs(char x);
ushort OVERLOADABLE abs(short x);
uint OVERLOADABLE abs(int x);
char OVERLOADABLE add_sat(char x, char y);
uchar OVERLOADABLE add_sat(uchar x, uchar y);
short OVERLOADABLE add_sat(short x, short y);
ushort OVERLOADABLE add_sat(ushort x, ushort y);
int OVERLOADABLE add_sat(int x, int y);
uint OVERLOADABLE add_sat(uint x, uint y);
uchar OVERLOADABLE ctz(uchar x);
ushort OVERLOADABLE ctz(ushort x);
uint OVERLOADABLE ctz(uint x);
int OVERLOADABLE mad24(int x, int y, int z);
uint OVERLOADABLE mad24(uint x, uint y, uint z);
char OVERLOADABLE mad_sat(char a, char b, char c);
uchar OVERLOADABLE mad_sat(uchar a, uchar b, uchar c);
short OVERLOADABLE mad_sat(short a, short b, short c);
ushort OVERLOADABLE mad_sat(ushort a, ushort b, ushort c);
int OVERLOADABLE mad_sat(int a, int b, int c);
uint OVERLOADABLE mad_sat(uint a, uint b, uint c);
char OVERLOADABLE max(char x, char y);
uchar OVERLOADABLE max(uchar x, uchar y);
short OVERLOADABLE max(short x, short y);
ushort OVERLOADABLE max(ushort x, ushort y);
int OVERLOADABLE max(int x, int y);
uint OVERLOADABLE max(uint x, uint y);
long OVERLOADABLE max(long x, long y);
ulong OVERLOADABLE max(ulong x, ulong y);
char OVERLOADABLE min(char x, char y);
uchar OVERLOADABLE min(uchar x, uchar y);
short OVERLOADABLE min(short x, short y);
ushort OVERLOADABLE min(ushort x, ushort y);
int OVERLOADABLE min(int x, int y);
uint OVERLOADABLE min(uint x, uint y);
long OVERLOADABLE min(long x, long y);
ulong OVERLOADABLE min(ulong x, ulong y);
int OVERLOADABLE min(ulong x, int y);
uint OVERLOADABLE min(ulong x, uint y);
uint OVERLOADABLE mul_hi(uint x, uint y);
int OVERLOADABLE mul_hi(int x, int y);
uchar OVERLOADABLE popcount(uchar x);
ushort OVERLOADABLE popcount(ushort x);
uint OVERLOADABLE popcount(uint x);
char OVERLOADABLE sub_sat(char x, char y);
uchar OVERLOADABLE sub_sat(uchar x, uchar y);
short OVERLOADABLE sub_sat(short x, short y);
ushort OVERLOADABLE sub_sat(ushort x, ushort y);
int OVERLOADABLE sub_sat(int x, int y);
uint OVERLOADABLE sub_sat(uint x, uint y);
float OVERLOADABLE acos(float x);
float OVERLOADABLE asin(float value);
float OVERLOADABLE atan(float value);
float OVERLOADABLE ceil(float x);
float OVERLOADABLE fabs(float x);
float OVERLOADABLE floor(float x);
float OVERLOADABLE rint(float x);
float OVERLOADABLE trunc(float x);
float OVERLOADABLE native_cos(float x);
float OVERLOADABLE native_exp2(float x);
float OVERLOADABLE native_log2(float x);
float OVERLOADABLE native_powr(float x, float y);
float OVERLOADABLE native_recip(float x);
float OVERLOADABLE native_rsqrt(float x);
float OVERLOADABLE native_sin(float x);
float OVERLOADABLE native_sqrt(float x);
float OVERLOADABLE native_tan(float x);
#ifdef cl_khr_fp16
half OVERLOADABLE clamp(half x, half minval, half maxval);
half OVERLOADABLE max(half x, half y);
half OVERLOADABLE min(half x, half y);
half OVERLOADABLE acos(half x);
half OVERLOADABLE asin(half x);
half OVERLOADABLE atan(half x);
half OVERLOADABLE ceil(half x);
half OVERLOADABLE fabs(half x);
half OVERLOADABLE floor(half x);
half OVERLOADABLE fma(half a, half b, half c);
half OVERLOADABLE mad(half a, half b, half c);
half OVERLOADABLE rint(half x);
half OVERLOADABLE trunc(half x);
half OVERLOADABLE native_cos(half x);
half OVERLOADABLE native_exp2(half x);
half OVERLOADABLE native_log2(half x);
half OVERLOADABLE native_powr(half x, half y);
half OVERLOADABLE native_recip(half x);
half OVERLOADABLE native_rsqrt(half x);
half OVERLOADABLE native_sin(half x);
half OVERLOADABLE native_sqrt(half x);
half OVERLOADABLE native_tan(half x);
#endif
#if defined(cl_khr_fp64)
double OVERLOADABLE clamp(double x, double minval, double maxval);
double OVERLOADABLE ceil(double x);
double OVERLOADABLE fabs(double x);
double OVERLOADABLE floor(double x);
double OVERLOADABLE trunc(double x);
double OVERLOADABLE max(double x, double y);
double OVERLOADABLE min(double x, double y);
double OVERLOADABLE native_sqrt(double x);
double OVERLOADABLE native_rsqrt(double x);
#endif // defined(cl_khr_fp64)

#endif // __IBIF_INTRINSICS_IMPL_H__
