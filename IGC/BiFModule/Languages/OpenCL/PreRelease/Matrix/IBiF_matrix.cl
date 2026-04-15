/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// uncomment for debugging to print friendly numbers
//
// convert bf16 to int
// int bf162int(short a) {
//     return convert_int(as_float(a << 16));
// }
//
// convert packed 2 bf16 numbers to int2
// int2 pbf162int2(int a) {
//     short2 ai = as_short2(a);
//     return (int2)(bf162int(ai.x), bf162int(ai.y));
// }
//

// Optimized implementation of Joint Matrix Load/Store built-ins
// Highest values indicate most preferable implementations, when given level of
// optimization is not avaialble due to platform capabilities or given
// combination of parameters next best implementation will be used.
#define SCALAR_IMPL      0 // Subgroup load/store for each item of the slice.
#define VECTOR_IMPL      1 // Block read/write per row/column of the slice.
#define VECTOR_CONT_IMPL 2 // Single block read/write for whole slice, where possible.
#define BLOCK2D_IMPL     3 // Single block read/write 2d operation, only on supported platforms (default).

typedef ushort __attribute__((ext_vector_type(32))) ushort32;
typedef uint   __attribute__((ext_vector_type(32))) uint32;
// Bitwidth of types
#define BITWIDTH_char  8
#define BITWIDTH_short 16
#define BITWIDTH_int   32
#define BITWIDTH_long  64
#define BITWIDTH__(type) BITWIDTH_##type
#define BITWIDTH(type) BITWIDTH__(type)

// Shape MxK macros - shape is a part of final builtin's name.
#define SHAPE_CONCAT(M, K) M##x##K
#define MANGLE_PREFETCH_NAME(sg, elem_bitwidth, shape) \
  __builtin_spriv_OpJointMatrixPrefetchINTEL##sg##_##shape##_i##elem_bitwidth

// Prefetch impl
#define DEFINE_PREFETCH_IMPL(sg, element_type, elem_bitwidth, M, K, shape) \
  INLINE void MANGLE_PREFETCH_NAME(sg, elem_bitwidth, shape) (char *mem, long stride, int cacheOpt) { \
    long offset = as_long(mem); \
    long baseoffset = offset & (~0x3f); \
    int width = (sizeof (element_type)) * stride - 1; \
    int pitch = width; \
    int height = M - 1; \
    long x = (offset - baseoffset) / (sizeof (element_type)); \
    int2 coords = (int2)(x, 0); \
    void __builtin_IB_subgroup_block_read_flat_prefetch_u##elem_bitwidth##_m##M##k##K##v1(long, int, int, int, int2, int); \
    __builtin_IB_subgroup_block_read_flat_prefetch_u##elem_bitwidth##_m##M##k##K##v1(baseoffset, width, height, pitch, coords, cacheOpt); \
  }

#define DEFINE_PREFETCH__(sg, element_type, elem_bitwidth, M, K, shape) \
  DEFINE_PREFETCH_IMPL(sg, element_type, elem_bitwidth, M, K, shape)

#define DEFINE_PREFETCH(sg, element_type, M, K) \
  DEFINE_PREFETCH__(sg, element_type, BITWIDTH(element_type), M, K, SHAPE_CONCAT(M, K))

// Prefetch define all combinations
#define DEFINE_PREFETCH_GROUP_MK(M, K) \
    DEFINE_PREFETCH(_SG16, char,  M, K) \
    DEFINE_PREFETCH(_SG16, short, M, K) \
    DEFINE_PREFETCH(_SG16, int,   M, K) \
    DEFINE_PREFETCH(_SG16, long,  M, K)
#define DEFINE_PREFETCH_GROUP_K(K) \
    DEFINE_PREFETCH_GROUP_MK(1,  K) \
    DEFINE_PREFETCH_GROUP_MK(2,  K) \
    DEFINE_PREFETCH_GROUP_MK(4,  K) \
    DEFINE_PREFETCH_GROUP_MK(8,  K) \
    DEFINE_PREFETCH_GROUP_MK(16, K) \
    DEFINE_PREFETCH_GROUP_MK(32, K)
DEFINE_PREFETCH_GROUP_K(8)
DEFINE_PREFETCH_GROUP_K(16)
DEFINE_PREFETCH_GROUP_K(32)
DEFINE_PREFETCH_GROUP_K(64)
DEFINE_PREFETCH_GROUP_K(128)
DEFINE_PREFETCH_GROUP_K(256)

/* get_coord() support: */

#define MANGLE_GETCOORD_NAME(layout, sg, elem_bitwidth, R, C) \
  __builtin_spirv_OpJointMatrixGetCoordINTEL_##layout##sg##_##R##x##C##_i##elem_bitwidth

/* Explanation of calculation for int8 and bf16 types
Let's say we are considering a JM of use::A, 8x32, of type i8, in Platform PVC.
with sub-group size 16.

<--------- 32----------------------------->
0 0 x x x x ..........................x x ^
0 o x x x x ..........................x x |
0 0 x x x x ..........................x x 8
0 0 x x x x ..........................x x |
..
0 0 x x x x ..........................x x v

As we divide the elements of the JM col-wise across WIs, each WI will have a
8x2 slice of the JM, and the number of elements held by each WI will be 16.
For example, in the above figure, the elements marked with a '0' is held by
work_item_0 of that subgroup. The next WI will be holding the next 2 cols
and so on..

Now let's look at the calculation. Let's say we are interested in getting the
small o item in work_item_0. The index here is 3. (Please note that index is
the argument of get_coord() call. And each WI has index running 0-15 in this
case, as they hold 16 elements (8x2))

So the calculation becomes:
sg_cols = (C * VF) / pack_factor --> (32 * 1) / 2 = 16
skip_factor = sg_size / sg_cols --> 16 / 16  = 1
row: (wi_id / sg_cols + index / pack_factor * skip_factor) * VF + index % VF --> (0 / 16 + 3 / 2 * 1) * 1 + 3 % 1 = 1
col: (wi_id % sg_cols * pack_factor + index % pack_factor) / VF --> (0 % 16 * 2 + 3 % 2) / 1 = 1

Now, why the index for this particular item is 3 and not 9? That is because
the slice is stored in row-major fashion. So if we have the slice like
the following for a WI:

0 0
1 *1*
2 2
3 3
4 4
5 5
6 6
7 7

The storage in memory will be: 0 0 1 1 2 2 ... 7 7
*/

// R - number of rows
// C - number of columns
// VF - VNNI Factor
#define DEFINE_GET_COORD(layout, sg, elem_bitwidth, contrib_bitwidth, R, C, VF) \
  INLINE int2 MANGLE_GETCOORD_NAME(layout, sg, elem_bitwidth, R, C) (int index) { \
    int sg_size = get_sub_group_size(); \
    int wi_id = get_sub_group_local_id(); \
    int pack_factor = contrib_bitwidth / elem_bitwidth; \
    int sg_cols = (C * VF) / pack_factor; \
    int skip_factor = sg_size / sg_cols; \
    int row = (wi_id / sg_cols + index / pack_factor * skip_factor) * VF + index % VF; \
    int col = (wi_id % sg_cols * pack_factor + index % pack_factor) / VF; \
    int2 result = (int2)(row, col); \
    return result; \
  }

// ------ PVC -------
// layout, sg, elem_bitwidth, contrib_bitwidth, R, C, VF
//int8
DEFINE_GET_COORD(PackedA, _SG16, 8, 16, 8, 32, 1)
DEFINE_GET_COORD(PackedB, _SG16, 8, 32, 32, 16, 4)

//bfloat16
DEFINE_GET_COORD(PackedA, _SG16, 16, 16, 8, 16, 1)
DEFINE_GET_COORD(PackedA, _SG16, 16, 16, 16, 16, 1)
DEFINE_GET_COORD(PackedB, _SG16, 16, 32, 16, 16, 2)

// Accumulator
DEFINE_GET_COORD(Accumulator, _SG16, 32, 32, 8, 16, 1)
DEFINE_GET_COORD(Accumulator, _SG16, 32, 32, 16, 16, 1)

// --------- XMX8 ------------
//int8
DEFINE_GET_COORD(PackedA, , 8, 32, 8, 32, 1)
DEFINE_GET_COORD(PackedB, , 8, 32, 32, 8, 4)

//bfloat16
DEFINE_GET_COORD(PackedA, , 16, 32, 8, 16, 1)
DEFINE_GET_COORD(PackedB, , 16, 32, 16, 8, 2)

// Accumulator
DEFINE_GET_COORD(Accumulator, , 32, 32, 8, 8, 1)

/* experimental large slice support: */

// MAD:

#define DEFINE_MAD_LARGE_SLICE(a_type_short, b_type_short, c_type_short, d_type_short, a_suffix, b_suffix, c_suffix, d_suffix, c_type, d_type) \
INLINE void __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_##a_type_short##_##b_type_short##_##c_type_short##_##d_type_short(__private char *a_ptr, __private char *b_ptr, __private char *raw_c_ptr, __private char *result) { \
    short16 a     = *( short16 *)a_ptr; \
    int8 b           = *( int8 *)b_ptr; \
    c_type##16 raw_c = *( c_type##16 *)raw_c_ptr; \
\
    short8 a0 = ( short8 )(a.s0, a.s1, a.s2, a.s3, a.s4, a.s5, a.s6, a.s7); \
    short8 a1 = ( short8 )(a.s8, a.s9, a.sa, a.sb, a.sc, a.sd, a.se, a.sf); \
\
    c_type##16 c = *( c_type##16 *)&raw_c; \
\
    c_type##8 c0 = ( c_type##8 )(c.s0, c.s1, c.s2, c.s3, c.s4, c.s5, c.s6, c.s7); \
    c_type##8 c1 = ( c_type##8 )(c.s8, c.s9, c.sa, c.sb, c.sc, c.sd, c.se, c.sf); \
\
    d_type##8 fres0 = __builtin_IB_sub_group16_fdpas_##d_suffix##_##c_suffix##_##a_suffix##_##b_suffix##_8_8(c0, a0, b); \
    d_type##8 fres1 = __builtin_IB_sub_group16_fdpas_##d_suffix##_##c_suffix##_##a_suffix##_##b_suffix##_8_8(c1, a1, b); \
\
    d_type##8 res0 = *( d_type##8 *)&fres0; \
    d_type##8 res1 = *( d_type##8 *)&fres1; \
\
    __private d_type##16 *dst = (__private d_type##16 *)result; \
    *dst = ( d_type##16 )(res0.s0, res0.s1, res0.s2, res0.s3, res0.s4, res0.s5, res0.s6, res0.s7, \
                   res1.s0, res1.s1, res1.s2, res1.s3, res1.s4, res1.s5, res1.s6, res1.s7); \
} \
\
INLINE void __builtin_spriv_OpJointMatrixMadINTEL_1x64x16_##a_type_short##_##b_type_short##_##c_type_short##_##d_type_short(__private char *a_ptr, __private char *b_ptr, __private char *c_ptr, __private char *d_ptr) { \
    short a = *(short *) a_ptr; \
\
    int8 b0 = *(int8 *) b_ptr; \
    int8 b1 = *(int8 *)(b_ptr + 1 * 16 * (sizeof (short))); \
    int8 b2 = *(int8 *)(b_ptr + 2 * 16 * (sizeof (short))); \
    int8 b3 = *(int8 *)(b_ptr + 3 * 16 * (sizeof (short))); \
\
    c_type c0 = *(c_type *)  c_ptr; \
    c_type c1 = *(c_type *) (c_ptr + 1 * (sizeof (c_type))); \
    c_type c2 = *(c_type *) (c_ptr + 2 * (sizeof (c_type))); \
    c_type c3 = *(c_type *) (c_ptr + 3 * (sizeof (c_type))); \
\
    d_type d0 = __builtin_IB_sub_group16_fdpas_##d_suffix##_##c_suffix##_##a_suffix##_##b_suffix##_8_1(c0, a, b0); \
    d_type d1 = __builtin_IB_sub_group16_fdpas_##d_suffix##_##c_suffix##_##a_suffix##_##b_suffix##_8_1(c1, a, b1); \
    d_type d2 = __builtin_IB_sub_group16_fdpas_##d_suffix##_##c_suffix##_##a_suffix##_##b_suffix##_8_1(c2, a, b2); \
    d_type d3 = __builtin_IB_sub_group16_fdpas_##d_suffix##_##c_suffix##_##a_suffix##_##b_suffix##_8_1(c3, a, b3); \
\
    __private d_type##4 *dst = (__private d_type##4 *)d_ptr; \
    *dst = (d_type##4 )(as_##d_type(d0), as_##d_type(d1), as_##d_type(d2), as_##d_type(d3)); \
} \
\
INLINE void __builtin_spriv_OpJointMatrixMadINTEL_32x64x16_##a_type_short##_##b_type_short##_##c_type_short##_##d_type_short(__private char *a_ptr, __private char *b_ptr, __private char *c_ptr, __private char *d_ptr) { \
    __private char *a0 = a_ptr; \
    __private char *a1 = a_ptr + 16 * (sizeof (short)); \
\
    __private char *b0 = b_ptr; \
    __private char *b1 = b_ptr + 1 * 16 * (sizeof (short)); \
    __private char *b2 = b_ptr + 2 * 16 * (sizeof (short)); \
    __private char *b3 = b_ptr + 3 * 16 * (sizeof (short)); \
\
    __private char *c0 = c_ptr + 0 * 16 * (sizeof (c_type)); \
    __private char *c1 = c_ptr + 2 * 16 * (sizeof (c_type)); \
    __private char *c2 = c_ptr + 4 * 16 * (sizeof (c_type)); \
    __private char *c3 = c_ptr + 6 * 16 * (sizeof (c_type)); \
    __private char *c4 = c_ptr + 1 * 16 * (sizeof (c_type)); \
    __private char *c5 = c_ptr + 3 * 16 * (sizeof (c_type)); \
    __private char *c6 = c_ptr + 5 * 16 * (sizeof (c_type)); \
    __private char *c7 = c_ptr + 7 * 16 * (sizeof (c_type)); \
\
    __private char *d0 = d_ptr + 0 * 16 * (sizeof (d_type)); \
    __private char *d1 = d_ptr + 2 * 16 * (sizeof (d_type)); \
    __private char *d2 = d_ptr + 4 * 16 * (sizeof (d_type)); \
    __private char *d3 = d_ptr + 6 * 16 * (sizeof (d_type)); \
    __private char *d4 = d_ptr + 1 * 16 * (sizeof (d_type)); \
    __private char *d5 = d_ptr + 3 * 16 * (sizeof (d_type)); \
    __private char *d6 = d_ptr + 5 * 16 * (sizeof (d_type)); \
    __private char *d7 = d_ptr + 7 * 16 * (sizeof (d_type)); \
\
    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_##a_type_short##_##b_type_short##_##c_type_short##_##d_type_short(a0, b0, c0, d0); \
    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_##a_type_short##_##b_type_short##_##c_type_short##_##d_type_short(a0, b1, c1, d1); \
    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_##a_type_short##_##b_type_short##_##c_type_short##_##d_type_short(a0, b2, c2, d2); \
    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_##a_type_short##_##b_type_short##_##c_type_short##_##d_type_short(a0, b3, c3, d3); \
\
    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_##a_type_short##_##b_type_short##_##c_type_short##_##d_type_short(a1, b0, c4, d4); \
    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_##a_type_short##_##b_type_short##_##c_type_short##_##d_type_short(a1, b1, c5, d5); \
    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_##a_type_short##_##b_type_short##_##c_type_short##_##d_type_short(a1, b2, c6, d6); \
    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_##a_type_short##_##b_type_short##_##c_type_short##_##d_type_short(a1, b3, c7, d7); \
} \
\
INLINE void __builtin_spriv_OpJointMatrixMadINTEL_32x64x32_##a_type_short##_##b_type_short##_##c_type_short##_##d_type_short(__private char *a_ptr, __private char *b_ptr, __private char *c_ptr, __private char *d_ptr) { \
    short8 a[8]; \
    int8 b[8]; \
    for (int i = 0; i < 8; i++) { \
        a[i] = *(short8 *)(a_ptr + i * 8 * (sizeof (short))); \
        b[i] = *(int8 *)(b_ptr + i * 8 * (sizeof (int))); \
    } \
\
    c_type##8 c[16]; \
    for (int i = 0; i < 16; i++) \
        c[i] = *( c_type##8 *)(c_ptr + i * 8 * (sizeof (c_type))); \
\
_Pragma("unroll") /* TODO: investigate, why not unrolling the loop causes wrong code generated*/ \
    for (int i = 0; i < 4; i++) { \
        for (int j = 0; j < 4; j++) { \
            c_type##8 d = __builtin_IB_sub_group16_fdpas_##c_suffix##_##c_suffix##_##a_suffix##_##b_suffix##_8_8(c[i + 4*j], a[i], b[2*j]); \
            *( d_type##8 *)(d_ptr + (i + 4*j) * 8 * (sizeof (d_type))) = __builtin_IB_sub_group16_fdpas_##d_suffix##_##c_suffix##_##a_suffix##_##b_suffix##_8_8(d, a[i + 4], b[2*j + 1]); \
        } \
    } \
} \
\
INLINE void __builtin_spriv_OpJointMatrixMadINTEL_1x64x32_##a_type_short##_##b_type_short##_##c_type_short##_##d_type_short(__private char *a_ptr, __private char *b_ptr, __private char *c_ptr, __private char *d_ptr) { \
    short a0 = *(short *)(a_ptr + 0 * (sizeof (short))); \
    short a1 = *(short *)(a_ptr + 1 * (sizeof (short))); \
\
    int8 b[8]; \
    for (int i = 0; i < 8; i++) \
        b[i] = *(int8 *)(b_ptr + i * 8 * (sizeof (int))); \
\
    c_type c[4]; \
    for (int i = 0; i < 4; i++) \
        c[i] = *(c_type *)(c_ptr + i * (sizeof (c_type))); \
\
    for (int i = 0; i < 4; i++) { \
        d_type d = __builtin_IB_sub_group16_fdpas_##d_suffix##_##c_suffix##_##a_suffix##_##b_suffix##_8_1(c[i], a0, b[2 * i]); \
        *(d_type *)(d_ptr + i * (sizeof (d_type))) = __builtin_IB_sub_group16_fdpas_##d_suffix##_##d_suffix##_##a_suffix##_##b_suffix##_8_1(d, a1, b[2 * i + 1]); \
    } \
}

#define DEFINE_MAD_LARGE_SLICE_32x32x16(a_type_short, b_type_short, a_suffix, b_suffix) \
INLINE void __builtin_spriv_OpJointMatrixMadINTEL_32x32x16_##a_type_short##_##b_type_short##_fp32_fp32(__private char *a_ptr, __private char *b_ptr, __private char *c_ptr, __private char *d_ptr) { \
    int8 a0 = *(int8 *)a_ptr; \
    int8 a1 = *(int8 *) (a_ptr + 1 * 16 * (sizeof (short))); \
    int8 a2 = *(int8 *) (a_ptr + 2 * 16 * (sizeof (short))); \
    int8 a3 = *(int8 *) (a_ptr + 3 * 16 * (sizeof (short))); \
\
    int8 b0 = *(int8 *)b_ptr; \
    int8 b1 = *(int8 *) (b_ptr + 1 * 16 * (sizeof (short))); \
    int8 b2 = *(int8 *) (b_ptr + 2 * 16 * (sizeof (short))); \
    int8 b3 = *(int8 *) (b_ptr + 3 * 16 * (sizeof (short))); \
\
    float8 c0 = *( float8 *) (c_ptr + 0 * 8 * (sizeof (int))); \
    float8 c1 = *( float8 *) (c_ptr + 4 * 8 * (sizeof (int))); \
    float8 c2 = *( float8 *) (c_ptr + 8 * 8 * (sizeof (int))); \
    float8 c3 = *( float8 *) (c_ptr + 12 * 8 * (sizeof (int))); \
    float8 c4 = *( float8 *) (c_ptr + 1 * 8 * (sizeof (int))); \
    float8 c5 = *( float8 *) (c_ptr + 5 * 8 * (sizeof (int))); \
    float8 c6 = *( float8 *) (c_ptr + 9 * 8 * (sizeof (int))); \
    float8 c7 = *( float8 *) (c_ptr + 13 * 8 * (sizeof (int))); \
    float8 c8 = *( float8 *) (c_ptr + 2 * 8 * (sizeof (int))); \
    float8 c9 = *( float8 *) (c_ptr + 6 * 8 * (sizeof (int))); \
    float8 c10 = *( float8 *) (c_ptr + 10 * 8 * (sizeof (int))); \
    float8 c11 = *( float8 *) (c_ptr + 14 * 8 * (sizeof (int))); \
    float8 c12 = *( float8 *) (c_ptr + 3 * 8 * (sizeof (int))); \
    float8 c13 = *( float8 *) (c_ptr + 7 * 8 * (sizeof (int))); \
    float8 c14 = *( float8 *) (c_ptr + 11 * 8 * (sizeof (int))); \
    float8 c15 = *( float8 *) (c_ptr + 15 * 8 * (sizeof (int))); \
\
    *( float8 *) (d_ptr + 0 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c0, a0, b0); \
    *( float8 *) (d_ptr + 4 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c1, a0, b1); \
    *( float8 *) (d_ptr + 8 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c2, a0, b2); \
    *( float8 *) (d_ptr + 12 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c3, a0, b3); \
    *( float8 *) (d_ptr + 1 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c4, a1, b0); \
    *( float8 *) (d_ptr + 5 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c5, a1, b1); \
    *( float8 *) (d_ptr + 9 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c6, a1, b2); \
    *( float8 *) (d_ptr + 13 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c7, a1, b3); \
    *( float8 *) (d_ptr + 2 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c8, a2, b0); \
    *( float8 *) (d_ptr + 6 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c9, a2, b1); \
    *( float8 *) (d_ptr + 10 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c10, a2, b2); \
    *( float8 *) (d_ptr + 14 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c11, a2, b3); \
    *( float8 *) (d_ptr + 3 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c12, a3, b0); \
    *( float8 *) (d_ptr + 7 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c13, a3, b1); \
    *( float8 *) (d_ptr + 11 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c14, a3, b2); \
    *( float8 *) (d_ptr + 15 * 8 * (sizeof (float))) = __builtin_IB_sub_group_fdpas_##a_suffix##_##b_suffix##_8_8(c15, a3, b3); \
}

// bfloat16
DEFINE_MAD_LARGE_SLICE(bf16, bf16, fp32, fp32, bf, bf, f, f, float, float)
DEFINE_MAD_LARGE_SLICE(bf16, bf16, fp32, bf16, bf, bf, f, bf, float, short)
DEFINE_MAD_LARGE_SLICE(bf16, bf16, bf16, fp32, bf, bf, bf, f, short, float)
DEFINE_MAD_LARGE_SLICE(bf16, bf16, bf16, bf16, bf, bf, bf, bf, short, short)
DEFINE_MAD_LARGE_SLICE_32x32x16(bf16, bf16, bf, bf)
// half
DEFINE_MAD_LARGE_SLICE(fp16, fp16, fp32, fp32, hf, hf, f, f, float, float)
DEFINE_MAD_LARGE_SLICE(fp16, fp16, fp32, fp16, hf, hf, f, hf, float, half)
DEFINE_MAD_LARGE_SLICE(fp16, fp16, fp16, fp32, hf, hf, hf, f, half, float)
DEFINE_MAD_LARGE_SLICE(fp16, fp16, fp16, fp16, hf, hf, hf, hf, half, half)
DEFINE_MAD_LARGE_SLICE_32x32x16(fp16, fp16, hf, hf)


// FillChecked implementation

#define MANGLE_FILLCHECKED_NAME(elem_bitwidth, contrib_bitwidth, K, WI_rows) \
  __builtin_spirv_OpJointMatrixFillCheckedINTEL_i##elem_bitwidth##_i##contrib_bitwidth##_k##K##_wi##WI_rows

#define DEFINE_FILLCHECKED_IMPL(element_type, elem_bitwidth, contrib_bitwidth, K, WI_rows) \
  INLINE void MANGLE_FILLCHECKED_NAME(elem_bitwidth, contrib_bitwidth, K, WI_rows) (__private char *dst, int y, int x, int height, int width, element_type value) { \
    int slid = get_sub_group_local_id(); \
    int sg_size = get_sub_group_size(); \
    int pack_factor = contrib_bitwidth / elem_bitwidth; \
    int col_sg_ratio = (sg_size * pack_factor) / K; \
    int M = (WI_rows * sg_size * pack_factor) / K; \
    __private element_type *wi_contrib = (__private element_type *) dst; \
    for (int i = 0; i < WI_rows; i++) { \
        int row, col; \
        if (col_sg_ratio != 0) { \
            /* sg_size * pack_factor >= matrix width */ \
            row = slid / K + i * col_sg_ratio; \
            col = slid % K; \
        } else { \
            /* sg_size * pack_factor < matrix width */ \
            row = i % M; \
            col = (i / M) * sg_size + slid; \
        } \
        wi_contrib[i] = col < width - x && row < height - y ? value : 0; \
    } \
}

#define DEFINE_FILLCHECKED__(element_type, elem_bitwidth, contrib_bitwidth, K, WI_rows) \
  DEFINE_FILLCHECKED_IMPL(element_type, elem_bitwidth, contrib_bitwidth, K, WI_rows)

#define DEFINE_FILLCHECKED(element_type, contrib_type, K, WI_rows) \
  DEFINE_FILLCHECKED__(element_type, BITWIDTH(element_type), BITWIDTH(contrib_type), K, WI_rows)

#define DEFINE_FILLCHECKED_K(element_type, contrib_type, K) \
  DEFINE_FILLCHECKED(element_type, contrib_type, K, 1) \
  DEFINE_FILLCHECKED(element_type, contrib_type, K, 2) \
  DEFINE_FILLCHECKED(element_type, contrib_type, K, 4) \
  DEFINE_FILLCHECKED(element_type, contrib_type, K, 8) \
  DEFINE_FILLCHECKED(element_type, contrib_type, K, 16) \
  DEFINE_FILLCHECKED(element_type, contrib_type, K, 32) \
  DEFINE_FILLCHECKED(element_type, contrib_type, K, 64) \
  DEFINE_FILLCHECKED(element_type, contrib_type, K, 128)

#define DEFINE_FILLCHECKED_CONTRIB(element_type, contrib_type) \
  DEFINE_FILLCHECKED_K(element_type, contrib_type, 8) \
  DEFINE_FILLCHECKED_K(element_type, contrib_type, 16) \
  DEFINE_FILLCHECKED_K(element_type, contrib_type, 32) \
  DEFINE_FILLCHECKED_K(element_type, contrib_type, 64)

#define DEFINE_FILLCHECKED_GROUP(element_type) \
  DEFINE_FILLCHECKED_CONTRIB(element_type, short) \
  DEFINE_FILLCHECKED_CONTRIB(element_type, int)

DEFINE_FILLCHECKED_GROUP(char)
DEFINE_FILLCHECKED_GROUP(short)
DEFINE_FILLCHECKED_GROUP(int)

// --------- Test Dump Load/Store built-ins -------------------
// These are simple linear (non-transforming) load/store intrinsics for
// cooperative matrix data. They are test-only intrinsics that allow isolating
// load vs store bugs in cooperative matrix roundtrip tests.
// Each WI writes/reads its contribution elements at the row-major position
// in the output buffer, producing SG-size-independent results.
// Memory layout: row-major order of contribution-type elements.
// Parameters:
//   wiRows:      number of contribution elements per WI (slice size)
//   contribCols: number of contribution-type columns in the matrix
//   contribBytes: byte size of one contribution element
//   stride:      row stride in number of original elements
//   elemBytes:   byte size of one original element

INLINE void __builtin_spriv_OpCooperativeMatrixTestDumpLoadINTEL(
    __private char *dst, char *mem, int wiRows, int contribCols, int contribBytes,
    int stride, int elemBytes) {
  int slid = get_sub_group_local_id();
  int sgSize = get_sub_group_size();
  int sg_cols = (contribCols < sgSize) ? contribCols : sgSize;
  int skip_factor = sgSize / sg_cols;
  for (int i = 0; i < wiRows; i++) {
    int row = slid / sg_cols + i * skip_factor;
    int col = slid % sg_cols;
    int byteOffset = row * stride * elemBytes + col * contribBytes;
    for (int b = 0; b < contribBytes; b++) {
      dst[i * contribBytes + b] = mem[byteOffset + b];
    }
  }
}

INLINE void __builtin_spriv_OpCooperativeMatrixTestDumpStoreINTEL(
    char *mem, __private char *src, int wiRows, int contribCols, int contribBytes,
    int stride, int elemBytes) {
  int slid = get_sub_group_local_id();
  int sgSize = get_sub_group_size();
  int sg_cols = (contribCols < sgSize) ? contribCols : sgSize;
  int skip_factor = sgSize / sg_cols;
  for (int i = 0; i < wiRows; i++) {
    int row = slid / sg_cols + i * skip_factor;
    int col = slid % sg_cols;
    int byteOffset = row * stride * elemBytes + col * contribBytes;
    for (int b = 0; b < contribBytes; b++) {
      mem[byteOffset + b] = src[i * contribBytes + b];
    }
  }
}
