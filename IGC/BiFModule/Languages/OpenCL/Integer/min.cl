/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE
char OVERLOADABLE min(char x, char y) { return (x <= y) ? x : y; }

INLINE
uchar OVERLOADABLE min(uchar x, uchar y) { return (x <= y) ? x : y; }

INLINE
short OVERLOADABLE min(short x, short y) { return (x <= y) ? x : y; }

INLINE
ushort OVERLOADABLE min(ushort x, ushort y) { return (x <= y) ? x : y; }

INLINE
int OVERLOADABLE min(int x, int y) { return (x <= y) ? x : y; }

INLINE
uint OVERLOADABLE min(uint x, uint y) { return (x <= y) ? x : y; }

INLINE
long OVERLOADABLE min(long x, long y) { return (x < y) ? x : y; }

INLINE
ulong OVERLOADABLE min(ulong x, ulong y) { return (x < y) ? x : y; }

// !!! TODO: Why do we need this function?
INLINE
int OVERLOADABLE min(ulong x, int y) { return ((int)x <= y) ? (int)x : y; }

// !!! TODO: Why do we need this function?
INLINE
uint OVERLOADABLE min(ulong x, uint y) { return ((uint)x <= y) ? (uint)x : y; }

GENERATE_VECTOR_FUNCTIONS_2ARGS(min, char, char)
GENERATE_VECTOR_FUNCTIONS_2ARGS(min, uchar, uchar)
GENERATE_VECTOR_FUNCTIONS_2ARGS(min, short, short)
GENERATE_VECTOR_FUNCTIONS_2ARGS(min, ushort, ushort)
GENERATE_VECTOR_FUNCTIONS_2ARGS(min, int, int)
GENERATE_VECTOR_FUNCTIONS_2ARGS(min, uint, uint)
GENERATE_VECTOR_FUNCTIONS_2ARGS(min, long, long)
GENERATE_VECTOR_FUNCTIONS_2ARGS(min, ulong, ulong)

GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(min, char, char, char)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(min, uchar, uchar, uchar)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(min, short, short, short)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(min, ushort, ushort, ushort)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(min, int, int, int)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(min, uint, uint, uint)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(min, long, long, long)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(min, ulong, ulong, ulong)
