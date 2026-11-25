/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE
char OVERLOADABLE max(char x, char y) { return (x >= y) ? x : y; }

INLINE
uchar OVERLOADABLE max(uchar x, uchar y) { return (x >= y) ? x : y; }

INLINE
short OVERLOADABLE max(short x, short y) { return (x >= y) ? x : y; }

INLINE
ushort OVERLOADABLE max(ushort x, ushort y) { return (x >= y) ? x : y; }

INLINE
int OVERLOADABLE max(int x, int y) { return (x >= y) ? x : y; }

INLINE
uint OVERLOADABLE max(uint x, uint y) { return (x >= y) ? x : y; }

INLINE
long OVERLOADABLE max(long x, long y) { return (x >= y) ? x : y; }

INLINE
ulong OVERLOADABLE max(ulong x, ulong y) { return (x >= y) ? x : y; }

GENERATE_VECTOR_FUNCTIONS_2ARGS(max, char, char)
GENERATE_VECTOR_FUNCTIONS_2ARGS(max, uchar, uchar)
GENERATE_VECTOR_FUNCTIONS_2ARGS(max, short, short)
GENERATE_VECTOR_FUNCTIONS_2ARGS(max, ushort, ushort)
GENERATE_VECTOR_FUNCTIONS_2ARGS(max, int, int)
GENERATE_VECTOR_FUNCTIONS_2ARGS(max, uint, uint)
GENERATE_VECTOR_FUNCTIONS_2ARGS(max, long, long)
GENERATE_VECTOR_FUNCTIONS_2ARGS(max, ulong, ulong)

GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(max, char, char, char)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(max, uchar, uchar, uchar)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(max, short, short, short)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(max, ushort, ushort, ushort)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(max, int, int, int)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(max, uint, uint, uint)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(max, long, long, long)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(max, ulong, ulong, ulong)
