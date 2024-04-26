/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Optimized implementation of Joint Matrix Load/Store built-ins
// Highest values indicate most preferable implementations, when given level of
// optimization is not avaialble due to platform capabilities or given
// combination of parameters next best implementation will be used.
#define SCALAR_IMPL      0 // Subgroup load/store for each item of the slice.
#define VECTOR_IMPL      1 // Block read/write per row/column of the slice.
#define VECTOR_CONT_IMPL 2 // Single block read/write for whole slice, where possible.
#define BLOCK2D_IMPL     3 // Single block read/write 2d operation, only on supported platforms (default).

// Matrix order
#define _ROW_MAJOR 0
#define _COL_MAJOR 1
#define _VNNI_TX   2

// Address space
#define AS_GENERIC 0
#define AS_LOCAL   1
#define AS_GLOBAL  2

// Matrix layout
#define _PackedA_RowMajor 0
#define _PackedB_RowMajor 1
#define _PackedB_ColumnMajor 2
#define _PackedB_PackedB 3
#define _Accumulator_RowMajor 4
#define _Accumulator_ColumnMajor 5

#define ATTRIBUTE_AS_GENERIC __global /* the branch using this will be dead,
                                         however we still need a valid address
                                         space specifier to make a call to
                                         block read/write BI. */
#define ATTRIBUTE_AS_LOCAL   __local
#define ATTRIBUTE_AS_GLOBAL  __global

// Index for row major layout is calculated based on that sub group size may be
// bigger than N.
// Arguments:
//   sg_cols: Number of contiguous columns held in the subgroup
//   skip_factor: n, where we include elements from every n-th row of the JM
//   to be part of the wi.  e.g for a Matrix
//     1 2 3 4
//     5 6 7 8
//     9 10 11 12
//     13 14 15 16
//    if skip_factor == 2, we will include items <1, 9> (every "2"nd row) in the
//    first WI, <2, 10> in the second WI and so on..
#define IND_ROW_MAJOR(slid, stride, skip_factor, i, sg_cols) ((slid/sg_cols + i*skip_factor)*stride + (slid%sg_cols))
#define IND_COL_MAJOR(slid, stride, skip_factor, i, sg_cols) ((slid/sg_cols + i*skip_factor) + (slid%sg_cols)*stride)
#define IND_VNNI_TX(slid, stride, skip_factor, i, sg_cols) (i + (slid * stride))

// no int7, int6, int5 types
#define VEC_TO_VEC16(type, vec) \
    (type##16)(vec.s0, vec.s1, vec.s2, vec.s3, vec.s4, vec.s5, vec.s6, vec.s7, vec.s8, vec.s9, vec.sA, vec.sB, vec.sC, vec.sD, vec.sE, vec.sF)
#define VEC_TO_VEC8(type, vec) \
    (type##8)(vec.s0, vec.s1, vec.s2, vec.s3, vec.s4, vec.s5, vec.s6, vec.s7)
#define VEC_TO_VEC7(type, vec) \
    (type##8)(vec.s0, vec.s1, vec.s2, vec.s3, vec.s4, vec.s5, vec.s6, 0)
#define VEC_TO_VEC6(type, vec) \
    (type##8)(vec.s0, vec.s1, vec.s2, vec.s3, vec.s4, vec.s5, 0, 0)
#define VEC_TO_VEC5(type, vec) \
    (type##8)(vec.s0, vec.s1, vec.s2, vec.s3, vec.s4, 0, 0, 0)
#define VEC_TO_VEC4(type, vec) (type##4)(vec.s0, vec.s1, vec.s2, vec.s3)
#define VEC_TO_VEC3(type, vec) (type##3)(vec.s0, vec.s1, vec.s2)
#define VEC_TO_VEC2(type, vec) (type##2)(vec.s0, vec.s1)
#define VEC_TO_VEC1(type, vec) (type)(vec)

// in case of store, we can not use uint3 with intel_sub_group_block_write4
// for size 32, assumption is resulting vector is the same as input vector, so no need to create new one.
#define VEC_TO_VEC_STORE32(type, vec) vec
#define VEC_TO_VEC_STORE16(type, vec) VEC_TO_VEC16(type, vec)
#define VEC_TO_VEC_STORE8(type, vec) VEC_TO_VEC8(type, vec)
#define VEC_TO_VEC_STORE7(type, vec) VEC_TO_VEC7(type, vec)
#define VEC_TO_VEC_STORE6(type, vec) VEC_TO_VEC6(type, vec)
#define VEC_TO_VEC_STORE5(type, vec) VEC_TO_VEC5(type, vec)
#define VEC_TO_VEC_STORE4(type, vec) VEC_TO_VEC4(type, vec)
#define VEC_TO_VEC_STORE3(type, vec) (type##4)(vec.s0, vec.s1, vec.s2, 0)
#define VEC_TO_VEC_STORE2(type, vec) VEC_TO_VEC2(type, vec)
#define VEC_TO_VEC_STORE1(type, vec) VEC_TO_VEC1(type, vec)

#define ARR_TO_VEC8(type, arr) \
    (type##8)(arr[0], arr[1], arr[2], arr[3], \
              arr[4], arr[5], arr[6], arr[7])

#define ARR_TO_VEC7(type, arr) \
    (type##8)(arr[0], arr[1], arr[2], arr[3], \
              arr[4], arr[5], arr[6], 0)

#define ARR_TO_VEC6(type, arr) \
    (type##8)(arr[0], arr[1], arr[2], arr[3], \
              arr[4], arr[5], 0,      0)

#define ARR_TO_VEC5(type, arr) \
    (type##8)(arr[0], arr[1], arr[2], arr[3], \
              arr[4], 0,      0,      0)

#define ARR_TO_VEC4(type, arr) \
    (type##4)(arr[0], arr[1], arr[2], arr[3])

#define ARR_TO_VEC3(type, arr) \
    (type##3)(arr[0], arr[1], arr[2])

#define ARR_TO_VEC2(type, arr) \
    (type##2)(arr[0], arr[1])

#define ARR_TO_VEC1(type, arr) \
    arr[0]

typedef ushort __attribute__((ext_vector_type(32))) ushort32;
typedef uint   __attribute__((ext_vector_type(32))) uint32;

#define OUT_VEC32(type) type##32
#define OUT_VEC16(type) type##16
#define OUT_VEC8(type) type##8
#define OUT_VEC7(type) type##8
#define OUT_VEC6(type) type##8
#define OUT_VEC5(type) type##8
#define OUT_VEC4(type) type##4
#define OUT_VEC3(type) type##3
#define OUT_VEC2(type) type##2
#define OUT_VEC1(type) type

#define OUT_STORE_VEC32(type) type##32
#define OUT_STORE_VEC16(type) type##16
#define OUT_STORE_VEC8(type) type##8
#define OUT_STORE_VEC7(type) type##8
#define OUT_STORE_VEC6(type) type##8
#define OUT_STORE_VEC5(type) type##8
#define OUT_STORE_VEC4(type) type##4
#define OUT_STORE_VEC3(type) type##4
#define OUT_STORE_VEC2(type) type##2
#define OUT_STORE_VEC1(type) type

// Math division macros
#define MATH_128_DIV_4 32
#define MATH_128_DIV_2 64
#define MATH_64_DIV_64 1
#define MATH_64_DIV_32 2
#define MATH_64_DIV_16 4
#define MATH_64_DIV_8 8
#define MATH_64_DIV_4 16
#define MATH_64_DIV_2 32
#define MATH_64_DIV_1 64
#define     MATH_32_DIV_32 1
#define     MATH_32_DIV_16 2
#define     MATH_32_DIV_8 4
#define     MATH_32_DIV_4 8
#define     MATH_32_DIV_2 16
#define     MATH_32_DIV_1 32
#define MATH_16_DIV_16 1
#define MATH_16_DIV_8 2
#define MATH_16_DIV_4 4
#define MATH_16_DIV_2 8
#define MATH_16_DIV_1 16
#define     MATH_8_DIV_8 1
#define     MATH_8_DIV_4 2
#define     MATH_8_DIV_2 4
#define     MATH_8_DIV_1 8
#define MATH_4_DIV_4 1
#define MATH_4_DIV_2 2
#define MATH_4_DIV_1 4
#define     MATH_2_DIV_2 1
#define     MATH_2_DIV_1 2
#define MATH_1_DIV_1 1
#define MATH_DIV__(a, b) MATH_##a##_DIV_##b
#define MATH_DIV(a, b) MATH_DIV__(a, b)

// Math multiplication macros
#define MATH_32_MUL_2 64
#define MATH_32_MUL_1 32
#define     MATH_16_MUL_4 64
#define     MATH_16_MUL_2 32
#define     MATH_16_MUL_1 16
#define MATH_8_MUL_4 32
#define MATH_8_MUL_2 16
#define MATH_8_MUL_1 8
#define     MATH_4_MUL_4 16
#define     MATH_4_MUL_2 8
#define     MATH_4_MUL_1 4
#define MATH_2_MUL_2 4
#define MATH_2_MUL_1 2
#define     MATH_1_MUL_1 1
#define MATH_MUL__(a, b) MATH_##a##_MUL_##b
#define MATH_MUL(a, b) MATH_MUL__(a, b)

// Bitwidth of types
#define BITWIDTH_char  8
#define BITWIDTH_short 16
#define BITWIDTH_int   32
#define BITWIDTH_long  64
#define BITWIDTH_uchar  8
#define BITWIDTH_ushort 16
#define BITWIDTH_uint   32
#define BITWIDTH_ulong  64
#define BITWIDTH__(type) BITWIDTH_##type
#define BITWIDTH(type) BITWIDTH__(type)

// Shape MxK macros - shape is a part of final builtin's name.
// There is special logic for vnni builtins: (M * vnni_factor)x(K / vnni_factor)
#define SHAPE_CONCAT(M, K) M##x##K
#define SHAPE_CONCAT_VNNI__(M, K) SHAPE_CONCAT(M, K)
#define SHAPE_CONCAT_VNNI(M, K, vnni_factor) SHAPE_CONCAT_VNNI__(MATH_MUL(M, vnni_factor), MATH_DIV(K, vnni_factor))

#define SHAPE_PackedA_RowMajor(       M, K, elem_bitwidth, contrib_bitwidth) SHAPE_CONCAT(M, K)
#define SHAPE_PackedB_RowMajor(       M, K, elem_bitwidth, contrib_bitwidth) SHAPE_CONCAT_VNNI(M, K, MATH_DIV(contrib_bitwidth, elem_bitwidth))
#define SHAPE_PackedB_ColumnMajor(    M, K, elem_bitwidth, contrib_bitwidth) SHAPE_CONCAT_VNNI(M, K, MATH_DIV(contrib_bitwidth, elem_bitwidth))
#define SHAPE_PackedB_PackedB(        M, K, elem_bitwidth, contrib_bitwidth) SHAPE_CONCAT_VNNI(M, K, MATH_DIV(contrib_bitwidth, elem_bitwidth))
#define SHAPE_Accumulator_RowMajor(   M, K, elem_bitwidth, contrib_bitwidth) SHAPE_CONCAT(M, K)
#define SHAPE_Accumulator_ColumnMajor(M, K, elem_bitwidth, contrib_bitwidth) SHAPE_CONCAT(M, K)
#define SHAPE(layout, M, K, element_type, contrib_type) SHAPE_##layout(M, K, BITWIDTH(element_type), BITWIDTH(contrib_type))

// Get original number of rows before VNNI transformation.
// R parameter is number of rows.
#define R_ORIG_(R, elem_bitwidth, contrib_bitwidth) MATH_MUL(R, MATH_DIV(contrib_bitwidth, elem_bitwidth))
#define R_ORIG(R, elem_type, contrib_type) R_ORIG_(R, BITWIDTH(elem_type), BITWIDTH(contrib_type))

// Get number of 2d block stores needed for a given number of rows.
// R parameter is number of rows.
#define GET_NUM_STORES_1  1
#define GET_NUM_STORES_2  1
#define GET_NUM_STORES_3  1
#define GET_NUM_STORES_4  1
#define GET_NUM_STORES_5  1
#define GET_NUM_STORES_6  1
#define GET_NUM_STORES_7  1
#define GET_NUM_STORES_8  1
#define GET_NUM_STORES_16 2
#define GET_NUM_STORES_32 4
#define GET_NUM_STORES(R) GET_NUM_STORES_##R

// Get number of 2d block stores needed for a given number of columns in VNNI layout
// assuming number of rows is 8.
// C parameter is number of columns.
#define GET_NUM_STORES_VNNI_16 1 // data type can be d8, d16, or d32
#define GET_NUM_STORES_VNNI_32 1 // data type can be d8, or d16
#define GET_NUM_STORES_VNNI_64 1 // data type is d8
#define GET_NUM_STORES_VNNI_128 4 // data type is d16
#define GET_NUM_STORES_VNNI(C) GET_NUM_STORES_VNNI_##C

#define GET_NUM_STORES_PackedA_RowMajor(R, C) GET_NUM_STORES(R)
#define GET_NUM_STORES_PackedB_PackedB(R, C) GET_NUM_STORES_VNNI(C)
#define GET_NUM_STORES_PackedB_RowMajor(R, C) GET_NUM_STORES(R)
#define GET_NUM_STORES_PackedB_ColumnMajor(R, C) GET_NUM_STORES(R)
#define GET_NUM_STORES_Accumulator_RowMajor(R, C) GET_NUM_STORES(R)
#define GET_NUM_STORES_Accumulator_ColumnMajor(R, C) GET_NUM_STORES(R)
#define GET_NUM_STORES_(layout, R, C) GET_NUM_STORES_##layout(R, C)

// Calculates the size of the offset of source/destination memory for load/store depending on layout and element/contrib types.
// For A, we are storing chunks of 8 rows in one store
// For B, if we load/store matrix which was already VNNI'ed (ROW_MAJOR load), we use contrib type for memory offset and 16 is width of block (shape is like 8x64 int)
//        if we load matrix which was not VNNI'ed (VNNI_TX load), we use element type for offset (shape is like 16x64 short)
// For C, we are storing chunks of 16 columns in one store
#define MEM_OFFSET_PackedA_RowMajor(     elem_type, contrib_type) (8 *  sizeof(elem_type) * stride)
#define MEM_OFFSET_PackedB_PackedB(      elem_type, contrib_type) (16 * sizeof(contrib_type))
#define MEM_OFFSET_PackedB_RowMajor(     elem_type, contrib_type) (16 * sizeof(elem_type))
#define MEM_OFFSET_Accumulator_RowMajor( elem_type, contrib_type) (16 * sizeof(contrib_type))

// Number of rows in a single 2d block store used in the name of built-in
// it is 16 for PackedB matrix, because B is in VNNI format
#define SPLIT_STORE_HEIGHT_PackedA_RowMajor 8
#define SPLIT_STORE_HEIGHT_PackedB_PackedB 16
#define SPLIT_STORE_HEIGHT(layout) SPLIT_STORE_HEIGHT_##layout

// layout can be PackedA_RowMajor, PackedB_ColumnMajor, PackedB_PackedB, etc.
// sg is empty for XMX8 and _SG16 for PVC
// elem_bitwidth is 8, 16 or 32
// shape is shape of the matrix, like 8x16 (MxK). There is special logic for vnni shapes
// WI_rows is the number of rows owned by each WI, which can be different from M e.g. for tf32
#define MANGLE_LOAD_NAME_AS_GENERIC(layout, sg, elem_bitwidth, shape, WI_rows) \
  __builtin_spriv_OpJointMatrixLoadINTEL_##layout##sg##_##shape##_i##elem_bitwidth##_##WI_rows##_generic_v8i8_pi32_i32

#define MANGLE_LOAD_NAME_AS_LOCAL(layout, sg, elem_bitwidth, shape, WI_rows) \
  __builtin_spriv_OpJointMatrixLoadINTEL_##layout##sg##_##shape##_i##elem_bitwidth##_##WI_rows##_local_v8i8_pi32_i32

#define MANGLE_LOAD_NAME_AS_GLOBAL(layout, sg, elem_bitwidth, shape, WI_rows) \
  __builtin_spriv_OpJointMatrixLoadINTEL_##layout##sg##_##shape##_i##elem_bitwidth##_##WI_rows##_global_v8i8_pi32_i32


#define SUB_GROUP_LOAD(readop, M, src, dst, stride, contrib_type) \
    __private contrib_type *wi_contrib = (__private contrib_type *)dst; \
    for (int i = 0; i < M; i++) \
        wi_contrib[i] = readop((src) + i * (stride));

#define SUB_GROUP_LOAD_PACK_32(M, src, dst, stride) \
    /* empty */

#define SUB_GROUP_LOAD_PACK_16(M, src, dst, stride) \
    __private int *wi_contrib = (__private int *)dst; \
    for (int i = 0; i < M; i++) { \
      ushort row0 = intel_sub_group_block_read_us((src) + 2 * i * (stride)); \
      ushort row1 = intel_sub_group_block_read_us((src) + (2 * i + 1) * (stride)); \
      wi_contrib[i] = as_int((ushort2)(row0, row1)); \
    }

#define SUB_GROUP_LOAD_PACK_8(M, src, dst, stride) \
    __private int *wi_contrib = (__private int *)dst; \
    for (int i = 0; i < M; i++) { \
      uchar row0 = intel_sub_group_block_read_uc((src) + 4 * i * (stride)); \
      uchar row1 = intel_sub_group_block_read_uc((src) + (4 * i + 1) * (stride)); \
      uchar row2 = intel_sub_group_block_read_uc((src) + (4 * i + 2) * (stride)); \
      uchar row3 = intel_sub_group_block_read_uc((src) + (4 * i + 3) * (stride)); \
      wi_contrib[i] = as_int((uchar4)(row0, row1, row2, row3)); \
    }

// variants for 32, 16, 7, 6, 5, 3 and 1 are only used to make the code compilable
#define DEFINE_BLOCK_RW_NAME32(rw, us) intel_sub_group_block_##rw##us##32
#define DEFINE_BLOCK_RW_NAME16(rw, us) intel_sub_group_block_##rw##us##16
#define DEFINE_BLOCK_RW_NAME8(rw, us) intel_sub_group_block_##rw##us##8
#define DEFINE_BLOCK_RW_NAME7(rw, us) intel_sub_group_block_##rw##us##8
#define DEFINE_BLOCK_RW_NAME6(rw, us) intel_sub_group_block_##rw##us##8
#define DEFINE_BLOCK_RW_NAME5(rw, us) intel_sub_group_block_##rw##us##8
#define DEFINE_BLOCK_RW_NAME4(rw, us) intel_sub_group_block_##rw##us##4
#define DEFINE_BLOCK_RW_NAME3(rw, us) intel_sub_group_block_##rw##us##4
#define DEFINE_BLOCK_RW_NAME2(rw, us) intel_sub_group_block_##rw##us##2
#define DEFINE_BLOCK_RW_NAME1(rw, us) intel_sub_group_block_##rw##us

#define DEFINE_BLOCK2D_RW_NAME(rw, tx, contrib_bitwidth, WI_rows, M, K) __builtin_IB_subgroup_block_##rw##_flat_cacheopts##tx##_u##contrib_bitwidth##_wi##WI_rows##_m##M##k##K##v1
#define DEFINE_BLOCK2D_TRANSPOSE_NAME(contrib_bitwidth, K) __builtin_IB_subgroup_block_read_flat_cacheopts_transpose_u##contrib_bitwidth##_k##K
#define DEFINE_BLOCK2D_VNNI_NAME(contrib_bitwidth, K) __builtin_IB_subgroup_block_read_flat_cacheopts_transform_u##contrib_bitwidth##_k##K

/* For platforms without SG16 JointMatrix support block2d is not available. The
 * implementation remains empty, will fallthrough to vector implementation. */
#define IMPLEMENT_BLOCK2D_LOAD_ROW_MAJOR_(element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, WI_rows, contrib_K) \
  /* not supported, fallthrough */
#define IMPLEMENT_BLOCK2D_LOAD_COL_MAJOR_(element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, WI_rows, contrib_K) \
  /* not supported, fallthrough */
#define IMPLEMENT_BLOCK2D_LOAD_VNNI_TX_(element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, WI_rows, contrib_K) \
  /* not supported, fallthrough */
#define IMPLEMENT_BLOCK2D_STORE_1(layout, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, store_height, contrib_K) \
  /* not supported, fallthrough */

// contrib_K - calculated in BLOCK2D loads; contrib_K = K/(contrib_bitwidth/elem_bitwidth);
//     Since contrib_type might be larger than element_type.
//     To load a proper ammout per WI we need to contrib_K that's derived from K but it's smaller for some configurations.

#define MAX_ROW_BYTES_2D_BLOCK_LOAD 64 // maximum per row size in bytes supported by 2D block load

#define IMPLEMENT_BLOCK2D_LOAD_SG16_ROW_MAJOR_(element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, WI_rows, contrib_K) \
  if (contrib_K*sizeof(contrib_type) <= MAX_ROW_BYTES_2D_BLOCK_LOAD) { /* For 2D loads (block2d width)*(data size) must be <= MAX_ROW_BYTES_2D_BLOCK_LOAD */ \
    long offset = as_long(mem); \
    long baseoffset = offset & (~0x3f); /* align to 64-byte */ \
    int width = (sizeof (element_type)) * stride - 1; /* in bytes */ \
    int pitch = width; /* JointMatrices are expected to be contiguous in memory, without padding at the end of a row */ \
    int height = M - 1; /* row count */ \
    long x = (offset - baseoffset) / (sizeof (contrib_type)); /* in elements */ \
    int2 coords = (int2)(x, 0); \
    OUT_VEC##WI_rows(u##contrib_type) DEFINE_BLOCK2D_RW_NAME(read, , contrib_bitwidth, WI_rows, M, contrib_K)(long, int, int, int, int2, int); \
    OUT_VEC##WI_rows(u##contrib_type) res = DEFINE_BLOCK2D_RW_NAME(read, , contrib_bitwidth, WI_rows, M, contrib_K)(baseoffset, width, height, pitch, coords, cacheOpt); \
    *(__private OUT_VEC##WI_rows(u##contrib_type) *)dst = res; \
    return; \
  }

#define IMPLEMENT_BLOCK2D_LOAD_SG16_COL_MAJOR_(element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, WI_rows, contrib_K) \
  if (contrib_K*sizeof(element_type) <= MAX_ROW_BYTES_2D_BLOCK_LOAD) { /* For 2D loads (block2d width)*(data size) must be <= MAX_ROW_BYTES_2D_BLOCK_LOAD */ \
    long offset = as_long(mem); \
    long baseoffset = offset & (~0x3f); /* align to 64-byte */ \
    int width = (sizeof (element_type)) * stride - 1; /* in bytes */ \
    int pitch = width; /* JointMatrices are expected to be contiguous in memory, without padding at the end of a row */ \
    int height = contrib_K - 1; /* column count */ \
    long x = (offset - baseoffset) / (sizeof (contrib_type)); /* in elements */ \
    int2 coords = (int2)(x, 0); \
    /* 2D block read transpose builtin requires K value _after_ the transpose operation is done - which is equal to M before the transpose */ \
    OUT_VEC8(u##contrib_type) DEFINE_BLOCK2D_TRANSPOSE_NAME(elem_bitwidth, M)(long, int, int, int, int2, int); \
    OUT_VEC8(u##contrib_type) res = DEFINE_BLOCK2D_TRANSPOSE_NAME(elem_bitwidth, M)(baseoffset, width, height, pitch, coords, cacheOpt); \
    *(__private OUT_VEC##M(u##contrib_type) *)dst = *(__private OUT_VEC##M(u##contrib_type) *)&res; \
    return; \
  }

#define IMPLEMENT_BLOCK2D_LOAD_SG16_VNNI_TX_(element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, WI_rows, contrib_K) \
  if (contrib_K*sizeof(element_type) <= MAX_ROW_BYTES_2D_BLOCK_LOAD) { /* For 2D loads (block2d width)*(data size) must be <= MAX_ROW_BYTES_2D_BLOCK_LOAD */ \
    long offset = as_long(mem); \
    long baseoffset = offset & (~0x3f); /* align to 64-byte */ \
    int width = (sizeof (element_type)) * stride - 1; /* in bytes */ \
    int pitch = width; /* JointMatrices are expected to be contiguous in memory, without padding at the end of a row */ \
    int height = K - 1; /* row count */ \
    long x = (offset - baseoffset) / (sizeof (element_type)); /* in elements */ \
    int2 coords = (int2)(x, 0); \
    OUT_VEC##M(u##contrib_type) DEFINE_BLOCK2D_VNNI_NAME(elem_bitwidth, contrib_K)(long, int, int, int, int2, int); \
    OUT_VEC##M(u##contrib_type) res = DEFINE_BLOCK2D_VNNI_NAME(elem_bitwidth, contrib_K)(baseoffset, width, height, pitch, coords, cacheOpt); \
    *(__private OUT_VEC##M(u##contrib_type) *)dst = res; \
    return; \
  }

#define IMPLEMENT_BLOCK2D_LOAD__(sg, order, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, WI_rows) \
  IMPLEMENT_BLOCK2D_LOAD##sg##order(element_type, elem_bitwidth, contrib_type, contrib_bitwidth, \
                                    M, K, WI_rows, MATH_DIV(K, MATH_DIV(contrib_bitwidth, elem_bitwidth)))

#define IMPLEMENT_BLOCK2D_LOAD(sg, order, element_type, contrib_type, M, K, WI_rows) \
  IMPLEMENT_BLOCK2D_LOAD__(sg, order, element_type, BITWIDTH(element_type), contrib_type, BITWIDTH(contrib_type), \
                           M, K, WI_rows)

// _1 suffix in the name indicates that the function is using 1 2d block store
// store_height is not used
#define IMPLEMENT_BLOCK2D_STORE_SG16_1(layout, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, store_height, contrib_K) \
    long offset = as_long(mem); \
    long baseoffset = offset & (~0x3f); /* align to 64-byte */ \
    int width = (sizeof (element_type)) * stride - 1; /* in bytes */ \
    int pitch = width; /* JointMatrices are expected to be contiguous in memory, without padding at the end of a row */ \
    int height = M - 1; /* row count */ \
    long x = (offset - baseoffset) / (sizeof (contrib_type)); /* in elements */ \
    int2 coords = (int2)(x, 0); \
    void DEFINE_BLOCK2D_RW_NAME(write, , contrib_bitwidth, M, M, contrib_K)(long, int, int, int, int2, OUT_VEC##M(u##contrib_type), int); \
    OUT_VEC##M(u##contrib_type) val = *(OUT_VEC##M(u##contrib_type) *)src; \
    DEFINE_BLOCK2D_RW_NAME(write, , contrib_bitwidth, M, M, contrib_K)(baseoffset, width, height, pitch, coords, val, cacheOpt); \
    return;

// _2 suffix in the name indicates that the function is using 2 2d block stores
// store_height is not used
#define IMPLEMENT_BLOCK2D_STORE_SG16_2(layout, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, store_height, contrib_K) \
    __private char *c0 = src + 0 * 8 * (sizeof (contrib_type)); \
    __private char *c1 = src + 1 * 8 * (sizeof (contrib_type)); \
\
    char *mem0 = mem; \
    char *mem1 = mem + 8 * (sizeof (element_type)) * stride; \
\
    __builtin_spriv_OpJointMatrixStoreINTEL_##layout##_SG16_8x16_i##elem_bitwidth##_8_global_pi64_v8i8(mem0, c0, stride, cacheOpt); \
    __builtin_spriv_OpJointMatrixStoreINTEL_##layout##_SG16_8x16_i##elem_bitwidth##_8_global_pi64_v8i8(mem1, c1, stride, cacheOpt); \
    return;

// _4 suffix in the name indicates that the function is using 4 2d block stores
// store_height is a height of one store built-in called from the implementation
#define IMPLEMENT_BLOCK2D_STORE_SG16_4(layout, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, store_height, contrib_K) \
    __private char *c0 = src + 0 * 8 * (sizeof (contrib_type)); \
    __private char *c1 = src + 1 * 8 * (sizeof (contrib_type)); \
    __private char *c2 = src + 2 * 8 * (sizeof (contrib_type)); \
    __private char *c3 = src + 3 * 8 * (sizeof (contrib_type)); \
\
    char *mem0 = mem; \
    char *mem1 = mem + 1 * MEM_OFFSET_##layout(element_type, contrib_type); \
    char *mem2 = mem + 2 * MEM_OFFSET_##layout(element_type, contrib_type); \
    char *mem3 = mem + 3 * MEM_OFFSET_##layout(element_type, contrib_type); \
\
    __builtin_spriv_OpJointMatrixStoreINTEL_##layout##_SG16_##store_height##x16_i##elem_bitwidth##_8_global_pi64_v8i8(mem0, c0, stride, cacheOpt); \
    __builtin_spriv_OpJointMatrixStoreINTEL_##layout##_SG16_##store_height##x16_i##elem_bitwidth##_8_global_pi64_v8i8(mem1, c1, stride, cacheOpt); \
    __builtin_spriv_OpJointMatrixStoreINTEL_##layout##_SG16_##store_height##x16_i##elem_bitwidth##_8_global_pi64_v8i8(mem2, c2, stride, cacheOpt); \
    __builtin_spriv_OpJointMatrixStoreINTEL_##layout##_SG16_##store_height##x16_i##elem_bitwidth##_8_global_pi64_v8i8(mem3, c3, stride, cacheOpt); \
    return;

// layout can be PackedA_RowMajor, PackedB_ColumnMajor, PackedB_PackedB, etc.
// sg is empty for XMX8 and _SG16 for PVC
// element_type is char for i8, short for i16 and int for i32
// [automatic] elem_bitwidth is the bitwidth of the elem_type, expected values are 8, 16 or 32
// contrib_type is int or short depending on available OpenCL extension API
//     Following needs to be true -> (sizeof contrib_type)*(SG size) == (sizeof element_type)*K.
//     We might use contrib_type that is different to element_type to handle sizes K values
//     that aren't equal to 8 for SG8 or 16 for SG16.
// [automatic] contrib_bitwidth is the bitwidth of the contrib_type, expected values are 16 or 32
// M is number of rows
// K is number of columns
// [automatic] shape is shape of the matrix, like 8x16 (MxK). There is special logic for vnni shapes.
// order is ROW_MAJOR, COL_MAJOR, VNNI_TX
// us is empty for int contrib type and _us for short contrib type.
// WI_rows is the number of rows owned by each WI, which can be different from M e.g. for tf32

#define DEFINE_LOAD_BLOCK2D_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows) \
    /* When M != WI_rows, only scenarios limited to i32 row major are supported */ \
    bool is32bitElemHalfMRowMajor = elem_bitwidth == 32 && WI_rows == M / 2 && order == _ROW_MAJOR; \
    if ((WI_rows == M || is32bitElemHalfMRowMajor) && BIF_FLAG_CTRL_GET(JointMatrixLoadStoreOpt) >= BLOCK2D_IMPL \
        && (M == 2 || M == 4 || M == 8 || M == 16 || M == 32) \
        && (order == _ROW_MAJOR || order == _VNNI_TX || (order == _COL_MAJOR && contrib_bitwidth == 32)) \
        ) { \
        IMPLEMENT_BLOCK2D_LOAD(sg, order##_, element_type, contrib_type, M, K, WI_rows) \
    }

#define DEFINE_LOAD_VECTORS_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows, address_space) \
    if (WI_rows == M && BIF_FLAG_CTRL_GET(JointMatrixLoadStoreOpt) >= VECTOR_CONT_IMPL \
        && stride == K && (M == 2 || M == 4 || M == 8) && order == _ROW_MAJOR \
        ) { \
        OUT_STORE_VEC##M(u##contrib_type) OVERLOADABLE DEFINE_BLOCK_RW_NAME##M(read, us)(const ATTRIBUTE_##address_space u##contrib_type *); \
        OUT_STORE_VEC##M(u##contrib_type) res = DEFINE_BLOCK_RW_NAME##M(read, us)((ATTRIBUTE_##address_space u##contrib_type *)mem); \
        *(__private OUT_VEC##M(u##contrib_type) *)dst = *(__private OUT_VEC##M(u##contrib_type) *)&res; \
        return; \
    } \
    if (WI_rows == M && BIF_FLAG_CTRL_GET(JointMatrixLoadStoreOpt) >= VECTOR_IMPL && (order == _ROW_MAJOR || order == _VNNI_TX) \
        && (M != 1 || sg_size != 32) \
        ) { \
        int pack_factor = sizeof (u##contrib_type) / sizeof (element_type); \
        stride = stride / pack_factor; \
        if (order == _VNNI_TX) { /* for VNNI_TX contrib_type should be int and elem_type should be char or short */ \
        SUB_GROUP_LOAD_PACK_##elem_bitwidth(M, (ATTRIBUTE_##address_space uint *)mem, dst, stride); \
        return; \
        } \
        SUB_GROUP_LOAD(intel_sub_group_block_read##us, M, (ATTRIBUTE_##address_space u##contrib_type *)mem, dst, stride, contrib_type); \
        return; \
    }

#define DEFINE_LOAD_SCALAR_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows) \
    contrib_type *ptr = (contrib_type *)mem; \
    int slid = get_sub_group_local_id(); \
    int pack_factor = sizeof (contrib_type) / sizeof (element_type); \
    stride = stride / pack_factor; \
    int sg_cols = K / pack_factor; \
    int skip_factor = sg_size / sg_cols; \
    __private contrib_type *wi_contrib = (__private contrib_type *)dst; \
    for (int i = 0; i < WI_rows; i++) { \
    if ( (i*skip_factor + slid/sg_cols) < M ) \
        wi_contrib[i] = ptr[IND##order(slid, stride, skip_factor, i, sg_cols)]; \
    else \
        wi_contrib[i] = 0; /*last even row for matrix with odd number of rows doesn't exist*/ \
    }

#define DEFINE_LOAD_IMPL_AS_GENERIC(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows) \
    INLINE void MANGLE_LOAD_NAME_AS_GENERIC(layout, sg, elem_bitwidth, shape, WI_rows) (__private char *dst, char *mem, long stride, int cacheOpt) { \
        int sg_size = get_sub_group_size(); \
        __builtin_assume((__global char*)mem != 0); \
        int memIsGlobal = (0 != SPIRV_BUILTIN(GenericCastToPtrExplicit, _p1i8_p4i8_i32, _ToGlobal)(__builtin_astype((mem), __generic char*), StorageWorkgroup)); \
        if (memIsGlobal) { \
            DEFINE_LOAD_BLOCK2D_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows) \
            DEFINE_LOAD_VECTORS_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows, AS_GLOBAL) \
        } else { \
            DEFINE_LOAD_VECTORS_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows, AS_LOCAL) \
        } \
        DEFINE_LOAD_SCALAR_IMPL( layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows) \
    }
#define DEFINE_LOAD_IMPL_AS_LOCAL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows) \
    INLINE void MANGLE_LOAD_NAME_AS_LOCAL(layout, sg, elem_bitwidth, shape, WI_rows) (__private char *dst, char *mem, long stride, int cacheOpt) { \
        int sg_size = get_sub_group_size(); \
        DEFINE_LOAD_VECTORS_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows, AS_LOCAL) \
        DEFINE_LOAD_SCALAR_IMPL( layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows) \
    }
#define DEFINE_LOAD_IMPL_AS_GLOBAL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows) \
    INLINE void MANGLE_LOAD_NAME_AS_GLOBAL(layout, sg, elem_bitwidth, shape, WI_rows) (__private char *dst, char *mem, long stride, int cacheOpt) { \
        int sg_size = get_sub_group_size(); \
        DEFINE_LOAD_BLOCK2D_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows) \
        DEFINE_LOAD_VECTORS_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows, AS_GLOBAL) \
        DEFINE_LOAD_SCALAR_IMPL( layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows) \
    }

#define DEFINE_LOAD_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows) \
    DEFINE_LOAD_IMPL_AS_GENERIC(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows) \
    DEFINE_LOAD_IMPL_AS_LOCAL(  layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows) \
    DEFINE_LOAD_IMPL_AS_GLOBAL( layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows)

#define DEFINE_LOAD(layout, sg, element_type, contrib_type, M, K, order, us, WI_rows) \
    DEFINE_LOAD_IMPL(layout, sg, element_type, BITWIDTH(element_type), contrib_type, BITWIDTH(contrib_type), \
                     M, K, SHAPE(layout, M, K, element_type, contrib_type), order, us, WI_rows)

/* PackedA load i16 */
DEFINE_LOAD(PackedA_RowMajor, , short, int, 8, 16, ROW_MAJOR, , 8)
DEFINE_LOAD(PackedA_RowMajor, , short, int, 7, 16, ROW_MAJOR, , 7)
DEFINE_LOAD(PackedA_RowMajor, , short, int, 6, 16, ROW_MAJOR, , 6)
DEFINE_LOAD(PackedA_RowMajor, , short, int, 5, 16, ROW_MAJOR, , 5)
DEFINE_LOAD(PackedA_RowMajor, , short, int, 4, 16, ROW_MAJOR, , 4)
DEFINE_LOAD(PackedA_RowMajor, , short, int, 3, 16, ROW_MAJOR, , 3)
DEFINE_LOAD(PackedA_RowMajor, , short, int, 2, 16, ROW_MAJOR, , 2)
DEFINE_LOAD(PackedA_RowMajor, , short, int, 1, 16, ROW_MAJOR, , 1)

/* PackedA load i8 */
DEFINE_LOAD(PackedA_RowMajor, , char, int, 8, 32, ROW_MAJOR, , 8)
DEFINE_LOAD(PackedA_RowMajor, , char, int, 7, 32, ROW_MAJOR, , 7)
DEFINE_LOAD(PackedA_RowMajor, , char, int, 6, 32, ROW_MAJOR, , 6)
DEFINE_LOAD(PackedA_RowMajor, , char, int, 5, 32, ROW_MAJOR, , 5)
DEFINE_LOAD(PackedA_RowMajor, , char, int, 4, 32, ROW_MAJOR, , 4)
DEFINE_LOAD(PackedA_RowMajor, , char, int, 3, 32, ROW_MAJOR, , 3)
DEFINE_LOAD(PackedA_RowMajor, , char, int, 2, 32, ROW_MAJOR, , 2)
DEFINE_LOAD(PackedA_RowMajor, , char, int, 1, 32, ROW_MAJOR, , 1)

/* PackedA load i16 SG16 */
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 8, 16, ROW_MAJOR, _us, 8)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 7, 16, ROW_MAJOR, _us, 7)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 6, 16, ROW_MAJOR, _us, 6)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 5, 16, ROW_MAJOR, _us, 5)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 4, 16, ROW_MAJOR, _us, 4)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 3, 16, ROW_MAJOR, _us, 3)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 2, 16, ROW_MAJOR, _us, 2)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 1, 16, ROW_MAJOR, _us, 1)

/* PackedA load i16 SG16 for sub group size = 32*/
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 8, 16, ROW_MAJOR, _us, 4)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 7, 16, ROW_MAJOR, _us, 4)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 6, 16, ROW_MAJOR, _us, 3)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 5, 16, ROW_MAJOR, _us, 3)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 4, 16, ROW_MAJOR, _us, 2)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 3, 16, ROW_MAJOR, _us, 2)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 2, 16, ROW_MAJOR, _us, 1)
// DEFINE_LOAD(PackedA_RowMajor, _SG16, short, short, 1, 16, ROW_MAJOR, _us, 1) same as for subgroup 16

/* PackedA load i8 SG16 */
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 8, 32, ROW_MAJOR, _us, 8)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 7, 32, ROW_MAJOR, _us, 7)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 6, 32, ROW_MAJOR, _us, 6)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 5, 32, ROW_MAJOR, _us, 5)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 4, 32, ROW_MAJOR, _us, 4)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 3, 32, ROW_MAJOR, _us, 3)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 2, 32, ROW_MAJOR, _us, 2)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 1, 32, ROW_MAJOR, _us, 1)

/* PackedA load i8 SG16 for sub group size 32*/
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 8, 32, ROW_MAJOR, _us, 4)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 7, 32, ROW_MAJOR, _us, 4)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 6, 32, ROW_MAJOR, _us, 3)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 5, 32, ROW_MAJOR, _us, 3)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 4, 32, ROW_MAJOR, _us, 2)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 3, 32, ROW_MAJOR, _us, 2)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 2, 32, ROW_MAJOR, _us, 1)
// DEFINE_LOAD(PackedA_RowMajor, _SG16, char, short, 1, 32, ROW_MAJOR, _us, 1)  same as for subgroup 16

/* A load tf32 SG16 */
DEFINE_LOAD(PackedA_RowMajor, _SG16, int, int, 8, 8, ROW_MAJOR, , 4)
/* A load tf32 SG16 for sub group size 32*/
DEFINE_LOAD(PackedA_RowMajor, _SG16, int, int, 8, 8, ROW_MAJOR, , 2)

/* PackedB load i16 */
DEFINE_LOAD(PackedB_ColumnMajor, , short, int, 8, 16, COL_MAJOR, , 8)
DEFINE_LOAD(PackedB_PackedB, ,     short, int, 8, 16, ROW_MAJOR, , 8)
DEFINE_LOAD(PackedB_RowMajor, ,    short, int, 8, 16, VNNI_TX,   , 8)

/* PackedB load i8 */
DEFINE_LOAD(PackedB_ColumnMajor, , char, int, 8, 32, COL_MAJOR, , 8)
DEFINE_LOAD(PackedB_PackedB,     , char, int, 8, 32, ROW_MAJOR, , 8)
DEFINE_LOAD(PackedB_RowMajor, ,    char, int, 8, 32, VNNI_TX,   , 8)

/* PackedB load i16 SG16 */
DEFINE_LOAD(PackedB_ColumnMajor, _SG16, short, int, 8, 32, COL_MAJOR, , 8)
DEFINE_LOAD(PackedB_PackedB,     _SG16, short, int, 8, 32, ROW_MAJOR, , 8)
DEFINE_LOAD(PackedB_RowMajor,    _SG16, short, int, 8, 32, VNNI_TX,   , 8)

/* PackedB load i16 for sub group size = 32*/
DEFINE_LOAD(PackedB_PackedB,     _SG16, short, int, 8, 32, ROW_MAJOR, , 4)

/* PackedB load i8 SG16*/
DEFINE_LOAD(PackedB_ColumnMajor, _SG16, char, int, 8, 64, COL_MAJOR, , 8)
DEFINE_LOAD(PackedB_PackedB,     _SG16, char, int, 8, 64, ROW_MAJOR, , 8)
DEFINE_LOAD(PackedB_RowMajor,    _SG16, char, int, 8, 64, VNNI_TX,   , 8)

/* PackedB load i8 SG16 for sub group size 32*/
DEFINE_LOAD(PackedB_PackedB,     _SG16, char, int, 8, 64, ROW_MAJOR, , 4)

/* B load tf32 SG16 */
DEFINE_LOAD(PackedB_RowMajor, _SG16, int, int, 8, 16, ROW_MAJOR, , 8)

/* B load tf32 SG16 sub group = 32 */
DEFINE_LOAD(PackedB_RowMajor, _SG16, int, int, 8, 16, ROW_MAJOR, , 4)

/* Load accumulator is a special case of load packed A, both are row major: */
DEFINE_LOAD(Accumulator_RowMajor, , int, int, 8, 8, ROW_MAJOR, , 8)
DEFINE_LOAD(Accumulator_RowMajor, , int, int, 7, 8, ROW_MAJOR, , 7)
DEFINE_LOAD(Accumulator_RowMajor, , int, int, 6, 8, ROW_MAJOR, , 6)
DEFINE_LOAD(Accumulator_RowMajor, , int, int, 5, 8, ROW_MAJOR, , 5)
DEFINE_LOAD(Accumulator_RowMajor, , int, int, 4, 8, ROW_MAJOR, , 4)
DEFINE_LOAD(Accumulator_RowMajor, , int, int, 3, 8, ROW_MAJOR, , 3)
DEFINE_LOAD(Accumulator_RowMajor, , int, int, 2, 8, ROW_MAJOR, , 2)
DEFINE_LOAD(Accumulator_RowMajor, , int, int, 1, 8, ROW_MAJOR, , 1)

/* Accumulator load i32 SG8 with transpose */
DEFINE_LOAD(Accumulator_ColumnMajor, , int, int, 8, 8, COL_MAJOR, , 8)
DEFINE_LOAD(Accumulator_ColumnMajor, , int, int, 7, 8, COL_MAJOR, , 7)
DEFINE_LOAD(Accumulator_ColumnMajor, , int, int, 6, 8, COL_MAJOR, , 6)
DEFINE_LOAD(Accumulator_ColumnMajor, , int, int, 5, 8, COL_MAJOR, , 5)
DEFINE_LOAD(Accumulator_ColumnMajor, , int, int, 4, 8, COL_MAJOR, , 4)
DEFINE_LOAD(Accumulator_ColumnMajor, , int, int, 3, 8, COL_MAJOR, , 3)
DEFINE_LOAD(Accumulator_ColumnMajor, , int, int, 2, 8, COL_MAJOR, , 2)
DEFINE_LOAD(Accumulator_ColumnMajor, , int, int, 1, 8, COL_MAJOR, , 1)

/* SG16*/
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 8, 16, ROW_MAJOR, , 8)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 7, 16, ROW_MAJOR, , 7)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 6, 16, ROW_MAJOR, , 6)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 5, 16, ROW_MAJOR, , 5)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 4, 16, ROW_MAJOR, , 4)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 3, 16, ROW_MAJOR, , 3)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 2, 16, ROW_MAJOR, , 2)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 1, 16, ROW_MAJOR, , 1)

/* Accumulator load i32 SG16 with transpose */
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 8, 16, COL_MAJOR, , 8)
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 7, 16, COL_MAJOR, , 7)
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 6, 16, COL_MAJOR, , 6)
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 5, 16, COL_MAJOR, , 5)
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 4, 16, COL_MAJOR, , 4)
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 3, 16, COL_MAJOR, , 3)
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 2, 16, COL_MAJOR, , 2)
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 1, 16, COL_MAJOR, , 1)

/* SG16 for subgroup 32*/
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 8, 16, ROW_MAJOR, , 4)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 7, 16, ROW_MAJOR, , 4)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 6, 16, ROW_MAJOR, , 3)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 5, 16, ROW_MAJOR, , 3)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 4, 16, ROW_MAJOR, , 2)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 3, 16, ROW_MAJOR, , 2)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 2, 16, ROW_MAJOR, , 1)
// DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, int, 1, 16, ROW_MAJOR, , 1) same as for subgroup 16

/* Accumulator load i32 SG16 for subgroup 32 with transpose */
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 8, 16, COL_MAJOR, , 4)
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 7, 16, COL_MAJOR, , 4)
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 6, 16, COL_MAJOR, , 3)
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 5, 16, COL_MAJOR, , 3)
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 4, 16, COL_MAJOR, , 2)
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 3, 16, COL_MAJOR, , 2)
DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 2, 16, COL_MAJOR, , 1)
// DEFINE_LOAD(Accumulator_ColumnMajor, _SG16, int, int, 1, 16, COL_MAJOR, , 1) same as for subgroup 16

// --------- STORE built-ins --------------------------------------

#define MANGLE_STORE_NAME_AS_GENERIC(layout, sg, elem_bitwidth, shape, WI_rows) \
  __builtin_spriv_OpJointMatrixStoreINTEL_##layout##sg##_##shape##_i##elem_bitwidth##_##WI_rows##_generic_pi64_v8i8
#define MANGLE_STORE_NAME_AS_LOCAL(layout, sg, elem_bitwidth, shape, WI_rows) \
  __builtin_spriv_OpJointMatrixStoreINTEL_##layout##sg##_##shape##_i##elem_bitwidth##_##WI_rows##_local_pi64_v8i8
#define MANGLE_STORE_NAME_AS_GLOBAL(layout, sg, elem_bitwidth, shape, WI_rows) \
  __builtin_spriv_OpJointMatrixStoreINTEL_##layout##sg##_##shape##_i##elem_bitwidth##_##WI_rows##_global_pi64_v8i8

#define VEC_IND8(var, ind) var[ind]
#define VEC_IND7(var, ind) var[ind]
#define VEC_IND6(var, ind) var[ind]
#define VEC_IND5(var, ind) var[ind]
#define VEC_IND4(var, ind) var[ind]
#define VEC_IND3(var, ind) var[ind]
#define VEC_IND2(var, ind) var[ind]
#define VEC_IND1(var, ind) var

// set block_opt to false to disable block non-continous optimization per one built-in as a workaround
// num_stores - how many block 2d store operations are needed to store the whole Joint Matrix of this shape
#define DEFINE_STORE_BLOCK2D_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows, block_opt, store_height, num_stores) \
    if (WI_rows >= M && BIF_FLAG_CTRL_GET(JointMatrixLoadStoreOpt) >= BLOCK2D_IMPL && (M == 2 || M == 4 || M == 8 || M == 16 || M == 32) \
        && order == _ROW_MAJOR && elem_bitwidth >= 8  \
        ) { \
        IMPLEMENT_BLOCK2D_STORE##sg##_##num_stores(layout, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, store_height, MATH_DIV(K, MATH_DIV(contrib_bitwidth, elem_bitwidth))) \
    }

#define DEFINE_STORE_VECTORS_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows, block_opt, store_height, num_stores, address_space) \
    if (WI_rows == M && BIF_FLAG_CTRL_GET(JointMatrixLoadStoreOpt) >= VECTOR_CONT_IMPL && stride == K \
        && (M == 2 || M == 4 || M == 8) && order == _ROW_MAJOR \
        ) { \
        OUT_VEC##M(u##contrib_type) vec = *(__private OUT_VEC##M(u##contrib_type) *)src; \
        void OVERLOADABLE DEFINE_BLOCK_RW_NAME##M(write, us)(ATTRIBUTE_##address_space u##contrib_type *, OUT_STORE_VEC##M(u##contrib_type)); \
        DEFINE_BLOCK_RW_NAME##M(write, us)((ATTRIBUTE_##address_space u##contrib_type *)mem, VEC_TO_VEC_STORE##M(u##contrib_type , vec)); \
        return; \
    } \
    if (WI_rows == M && (BIF_FLAG_CTRL_GET(JointMatrixLoadStoreOpt) >= VECTOR_IMPL) \
        && order == _ROW_MAJOR && block_opt == true \
        && (M != 1 || sg_size != 32) \
        ) { \
        ATTRIBUTE_##address_space u##contrib_type *ptr = (ATTRIBUTE_##address_space u##contrib_type *)mem; \
        int pack_factor = sizeof (u##contrib_type) / sizeof (element_type); \
        stride = stride / pack_factor; \
        for (int i = 0; i < M; i++) \
            intel_sub_group_block_write##us(ptr + i * stride, ((__private u##contrib_type *)src)[i]); \
        return; \
    }

#define DEFINE_STORE_SCALAR_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows, block_opt, store_height, num_stores) \
    contrib_type *ptr = (contrib_type *)mem; \
    int slid = get_sub_group_local_id(); \
    int pack_factor = sizeof (contrib_type) / sizeof (element_type); \
    stride = stride / pack_factor; \
    int sg_cols = K / pack_factor; \
    int skip_factor = sg_size / sg_cols; \
    __private contrib_type *slice = (__private contrib_type *)src; \
    for (int i = 0; i < WI_rows; i++) { \
    if ( (i*skip_factor + slid/sg_cols) < M ) \
        ptr[IND##order(slid, stride, skip_factor, i, sg_cols)] = slice[i]; \
    else \
        continue; /*last even row for matrix with odd number of rows doesn't exist*/ \
    }

#define DEFINE_STORE_IMPL_AS_GENERIC(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows, block_opt, store_height, num_stores) \
    INLINE void MANGLE_STORE_NAME_AS_GENERIC(layout, sg, elem_bitwidth, shape, WI_rows) (char *mem, __private char *src, long stride, int cacheOpt) { \
        int sg_size = get_sub_group_size(); \
        __builtin_assume((__global char*)mem != 0); \
        int memIsGlobal = (0 != SPIRV_BUILTIN(GenericCastToPtrExplicit, _p1i8_p4i8_i32, _ToGlobal)(__builtin_astype((mem), __generic char*), StorageWorkgroup)); \
        if (memIsGlobal) { \
            DEFINE_STORE_BLOCK2D_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows, block_opt, store_height, num_stores) \
            DEFINE_STORE_VECTORS_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows, block_opt, store_height, num_stores, AS_GLOBAL) \
        } else { \
            DEFINE_STORE_VECTORS_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows, block_opt, store_height, num_stores, AS_LOCAL) \
        } \
        DEFINE_STORE_SCALAR_IMPL( layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows, block_opt, store_height, num_stores) \
    }
#define DEFINE_STORE_IMPL_AS_LOCAL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows, block_opt, store_height, num_stores) \
    INLINE void MANGLE_STORE_NAME_AS_LOCAL(layout, sg, elem_bitwidth, shape, WI_rows) (char *mem, __private char *src, long stride, int cacheOpt) { \
        int sg_size = get_sub_group_size(); \
        DEFINE_STORE_VECTORS_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows, block_opt, store_height, num_stores, AS_LOCAL) \
        DEFINE_STORE_SCALAR_IMPL( layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows, block_opt, store_height, num_stores) \
    }
#define DEFINE_STORE_IMPL_AS_GLOBAL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows, block_opt, store_height, num_stores) \
    INLINE void MANGLE_STORE_NAME_AS_GLOBAL(layout, sg, elem_bitwidth, shape, WI_rows) (char *mem, __private char *src, long stride, int cacheOpt) { \
        int sg_size = get_sub_group_size(); \
        DEFINE_STORE_BLOCK2D_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows, block_opt, store_height, num_stores) \
        DEFINE_STORE_VECTORS_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows, block_opt, store_height, num_stores, AS_GLOBAL) \
        DEFINE_STORE_SCALAR_IMPL( layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, WI_rows, block_opt, store_height, num_stores) \
    }

#define DEFINE_STORE_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows, block_opt, num_stores) \
    DEFINE_STORE_IMPL_AS_GENERIC(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows, block_opt, SPLIT_STORE_HEIGHT(layout), num_stores) \
    DEFINE_STORE_IMPL_AS_LOCAL(  layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows, block_opt, SPLIT_STORE_HEIGHT(layout), num_stores) \
    DEFINE_STORE_IMPL_AS_GLOBAL( layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, WI_rows, block_opt, SPLIT_STORE_HEIGHT(layout), num_stores)

#define DEFINE_STORE(layout, sg, element_type, contrib_type, M, K, order, us, WI_rows, block_opt) \
    DEFINE_STORE_IMPL(layout, sg, element_type, BITWIDTH(element_type), contrib_type, BITWIDTH(contrib_type),\
                      M, K, SHAPE(layout, M, K, element_type, contrib_type), \
                      order, us, WI_rows, block_opt, GET_NUM_STORES_(layout, M, K))

// TODO: investigate why intel_sub_group_block_write causes an assertion and enable blocked non-continuous optimization

/* PackedA store i8 */
DEFINE_STORE(PackedA_RowMajor,      , char, int,   1, 32, ROW_MAJOR,    , 1, false)
DEFINE_STORE(PackedA_RowMajor,      , char, int,   2, 32, ROW_MAJOR,    , 2, false)
DEFINE_STORE(PackedA_RowMajor,      , char, int,   3, 32, ROW_MAJOR,    , 3, false)
DEFINE_STORE(PackedA_RowMajor,      , char, int,   4, 32, ROW_MAJOR,    , 4, false)
DEFINE_STORE(PackedA_RowMajor,      , char, int,   5, 32, ROW_MAJOR,    , 5, false)
DEFINE_STORE(PackedA_RowMajor,      , char, int,   6, 32, ROW_MAJOR,    , 6, false)
DEFINE_STORE(PackedA_RowMajor,      , char, int,   7, 32, ROW_MAJOR,    , 7, false)
DEFINE_STORE(PackedA_RowMajor,      , char, int,   8, 32, ROW_MAJOR,    , 8, false)

/* PackedA store i16 */
DEFINE_STORE(PackedA_RowMajor,      , short, int,   1, 16, ROW_MAJOR,    , 1, false)
DEFINE_STORE(PackedA_RowMajor,      , short, int,   2, 16, ROW_MAJOR,    , 2, false)
DEFINE_STORE(PackedA_RowMajor,      , short, int,   3, 16, ROW_MAJOR,    , 3, false)
DEFINE_STORE(PackedA_RowMajor,      , short, int,   4, 16, ROW_MAJOR,    , 4, false)
DEFINE_STORE(PackedA_RowMajor,      , short, int,   5, 16, ROW_MAJOR,    , 5, false)
DEFINE_STORE(PackedA_RowMajor,      , short, int,   6, 16, ROW_MAJOR,    , 6, false)
DEFINE_STORE(PackedA_RowMajor,      , short, int,   7, 16, ROW_MAJOR,    , 7, false)
DEFINE_STORE(PackedA_RowMajor,      , short, int,   8, 16, ROW_MAJOR,    , 8, false)

/* PackedA store i8 SG16 */
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 1, 32, ROW_MAJOR, _us, 1, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 2, 32, ROW_MAJOR, _us, 2, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 3, 32, ROW_MAJOR, _us, 3, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 4, 32, ROW_MAJOR, _us, 4, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 5, 32, ROW_MAJOR, _us, 5, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 6, 32, ROW_MAJOR, _us, 6, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 7, 32, ROW_MAJOR, _us, 7, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 8, 32, ROW_MAJOR, _us, 8, false)

/* PackedA store i8 SG16 for subgroup 32*/
// DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 1, 32, ROW_MAJOR, _us, 1, false) same as for subgroup 16
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 2, 32, ROW_MAJOR, _us, 1, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 3, 32, ROW_MAJOR, _us, 2, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 4, 32, ROW_MAJOR, _us, 2, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 5, 32, ROW_MAJOR, _us, 3, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 6, 32, ROW_MAJOR, _us, 3, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 7, 32, ROW_MAJOR, _us, 4, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  short, 8, 32, ROW_MAJOR, _us, 4, false)

/* PackedA store i16 SG16 */
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 1, 16, ROW_MAJOR, _us, 1, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 2, 16, ROW_MAJOR, _us, 2, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 3, 16, ROW_MAJOR, _us, 3, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 4, 16, ROW_MAJOR, _us, 4, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 5, 16, ROW_MAJOR, _us, 5, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 6, 16, ROW_MAJOR, _us, 6, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 7, 16, ROW_MAJOR, _us, 7, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 8, 16, ROW_MAJOR, _us, 8, false)

/* PackedA store i16 SG16 for sub group size 32 */
// DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 1, 16, ROW_MAJOR, _us, 1, false) same as for subgroup 16
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 2, 16, ROW_MAJOR, _us, 1, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 3, 16, ROW_MAJOR, _us, 2, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 4, 16, ROW_MAJOR, _us, 2, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 5, 16, ROW_MAJOR, _us, 3, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 6, 16, ROW_MAJOR, _us, 3, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 7, 16, ROW_MAJOR, _us, 4, false)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, short, 8, 16, ROW_MAJOR, _us, 4, false)

/* A store tf32 SG16 */
DEFINE_STORE(PackedA_RowMajor, _SG16, int, int, 8, 8, ROW_MAJOR, , 4, false)
/* A store tf32 SG16 for sub group size 32*/
DEFINE_STORE(PackedA_RowMajor, _SG16, int, int, 8, 8, ROW_MAJOR, , 2, false)

/* PackedB store i16*/
DEFINE_STORE(PackedB_ColumnMajor, , short, int, 8, 16, COL_MAJOR, , 8, false)
DEFINE_STORE(PackedB_PackedB,     , short, int, 8, 16, ROW_MAJOR, , 8, true)

/* PackedB store i16 SG16*/
DEFINE_STORE(PackedB_ColumnMajor, _SG16, short, int, 8, 32, COL_MAJOR, , 8, false)
DEFINE_STORE(PackedB_PackedB,     _SG16, short, int, 8, 32, ROW_MAJOR, , 8, true)

/* PackedB store i16 SG16 for subgroup 32*/
DEFINE_STORE(PackedB_PackedB,     _SG16, short, int, 8, 32, ROW_MAJOR, , 4, true)

// TODO: investigate why intel_sub_group_block_write causes an assertion and enable blocked non-continuous optimization
/* PackedB store i8 */
DEFINE_STORE(PackedB_ColumnMajor, , char, int, 8, 32, COL_MAJOR, , 8, false)
DEFINE_STORE(PackedB_PackedB,     , char, int, 8, 32, ROW_MAJOR, , 8, false)

/* PackedB store i8 SG16 */
DEFINE_STORE(PackedB_ColumnMajor, _SG16, char, int, 8, 64, COL_MAJOR, , 8, false)
DEFINE_STORE(PackedB_PackedB,     _SG16, char, int, 8, 64, ROW_MAJOR, , 8, false)

/* PackedB store i8 SG16 for subgroup 32*/
DEFINE_STORE(PackedB_PackedB,     _SG16, char, int, 8, 64, ROW_MAJOR, , 4, true)

/* B store tf32 SG16 */
DEFINE_STORE(PackedB_RowMajor, _SG16, int, int, 8, 16, ROW_MAJOR, , 8, true)

/* B store tf32 SG16 for sub group size 32 */
DEFINE_STORE(PackedB_RowMajor, _SG16, int, int, 8, 16, ROW_MAJOR, , 4, true)

/* Acc i32 */
DEFINE_STORE(Accumulator_RowMajor, , int, int, 8, 8, ROW_MAJOR, , 8, true)
DEFINE_STORE(Accumulator_RowMajor, , int, int, 7, 8, ROW_MAJOR, , 7, true)
DEFINE_STORE(Accumulator_RowMajor, , int, int, 6, 8, ROW_MAJOR, , 6, true)
DEFINE_STORE(Accumulator_RowMajor, , int, int, 5, 8, ROW_MAJOR, , 5, true)
DEFINE_STORE(Accumulator_RowMajor, , int, int, 4, 8, ROW_MAJOR, , 4, true)
DEFINE_STORE(Accumulator_RowMajor, , int, int, 3, 8, ROW_MAJOR, , 3, true)
DEFINE_STORE(Accumulator_RowMajor, , int, int, 2, 8, ROW_MAJOR, , 2, true)
DEFINE_STORE(Accumulator_RowMajor, , int, int, 1, 8, ROW_MAJOR, , 1, true)

/* Accumulator store i32 SG8 with transpose */
DEFINE_STORE(Accumulator_ColumnMajor, , int, int, 8, 8, COL_MAJOR, , 8, true)
DEFINE_STORE(Accumulator_ColumnMajor, , int, int, 7, 8, COL_MAJOR, , 7, true)
DEFINE_STORE(Accumulator_ColumnMajor, , int, int, 6, 8, COL_MAJOR, , 6, true)
DEFINE_STORE(Accumulator_ColumnMajor, , int, int, 5, 8, COL_MAJOR, , 5, true)
DEFINE_STORE(Accumulator_ColumnMajor, , int, int, 4, 8, COL_MAJOR, , 4, true)
DEFINE_STORE(Accumulator_ColumnMajor, , int, int, 3, 8, COL_MAJOR, , 3, true)
DEFINE_STORE(Accumulator_ColumnMajor, , int, int, 2, 8, COL_MAJOR, , 2, true)
DEFINE_STORE(Accumulator_ColumnMajor, , int, int, 1, 8, COL_MAJOR, , 1, true)

/* Acc i32 SG16 */
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 8, 16, ROW_MAJOR, , 8, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 7, 16, ROW_MAJOR, , 7, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 6, 16, ROW_MAJOR, , 6, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 5, 16, ROW_MAJOR, , 5, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 4, 16, ROW_MAJOR, , 4, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 3, 16, ROW_MAJOR, , 3, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 2, 16, ROW_MAJOR, , 2, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 1, 16, ROW_MAJOR, , 1, true)

/* Accumulator store i32 SG16 with transpose */
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 8, 16, COL_MAJOR, , 8, true)
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 7, 16, COL_MAJOR, , 7, true)
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 6, 16, COL_MAJOR, , 6, true)
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 5, 16, COL_MAJOR, , 5, true)
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 4, 16, COL_MAJOR, , 4, true)
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 3, 16, COL_MAJOR, , 3, true)
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 2, 16, COL_MAJOR, , 2, true)
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 1, 16, COL_MAJOR, , 1, true)

/* Acc i32 SG16 for subgroup 32*/
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 8, 16, ROW_MAJOR, , 4, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 7, 16, ROW_MAJOR, , 4, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 6, 16, ROW_MAJOR, , 3, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 5, 16, ROW_MAJOR, , 3, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 4, 16, ROW_MAJOR, , 2, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 3, 16, ROW_MAJOR, , 2, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 2, 16, ROW_MAJOR, , 1, true)
// DEFINE_STORE(Accumulator_RowMajor, _SG16, int, int, 1, 16, ROW_MAJOR, , 1, true) same as for subgroup 16

/* Accumulator store i32 SG16 for subgroup 32 with transpose */
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 8, 16, COL_MAJOR, , 4, true)
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 7, 16, COL_MAJOR, , 4, true)
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 6, 16, COL_MAJOR, , 3, true)
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 5, 16, COL_MAJOR, , 3, true)
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 4, 16, COL_MAJOR, , 2, true)
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 3, 16, COL_MAJOR, , 2, true)
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 2, 16, COL_MAJOR, , 1, true)
// DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, int, 1, 16, COL_MAJOR, , 1, true) same as for subgroup 16

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
row: (wi_id*pack_factor)/K + index/pack_factor*skip_factor  --> (0*2)/32 + 3/2*1 = 0 + 1 = 1
col: (wi_id*pack_factor)%K + index%pack_factor --> (0*2)%32 + 3%2 = 0 + 1 = 1

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
    int sg_cols = (C*VF) / pack_factor; \
    int skip_factor = sg_size / sg_cols; \
    int row = ((wi_id*pack_factor)/(C*VF) + index/pack_factor*skip_factor)* VF; \
    int col = ((wi_id * pack_factor) % (C*VF) + index % pack_factor)/ VF; \
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

#define DEFINE_MAD_16x16x16_IMPL(a_type, b_type, a_suffix, b_suffix) \
INLINE void __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_##a_type##_##b_type##_fp32(__private char *a_ptr, __private char *b_ptr, __private char *raw_c_ptr, __private char *result) { \
    short16 a   = *(short16 *)a_ptr; \
    int8 b      = *(int8 *)b_ptr; \
    int16 raw_c = *(int16 *)raw_c_ptr; \
\
    short8 a0 = (short8)(a.s0, a.s1, a.s2, a.s3, a.s4, a.s5, a.s6, a.s7); \
    short8 a1 = (short8)(a.s8, a.s9, a.sa, a.sb, a.sc, a.sd, a.se, a.sf); \
\
    float16 c = *(float16 *)&raw_c; \
\
    float8 c0 = (float8)(c.s0, c.s1, c.s2, c.s3, c.s4, c.s5, c.s6, c.s7); \
    float8 c1 = (float8)(c.s8, c.s9, c.sa, c.sb, c.sc, c.sd, c.se, c.sf); \
\
    float8 fres0 = __builtin_IB_sub_group16_fdpas_f_f_##a_suffix##_##b_suffix##_8_8(c0, a0, b); \
    float8 fres1 = __builtin_IB_sub_group16_fdpas_f_f_##a_suffix##_##b_suffix##_8_8(c1, a1, b); \
\
    int8 res0 = *(int8 *)&fres0; \
    int8 res1 = *(int8 *)&fres1; \
\
    __private int16 *dst = (__private int16 *)result; \
    *dst = (int16)(res0.s0, res0.s1, res0.s2, res0.s3, res0.s4, res0.s5, res0.s6, res0.s7, \
                   res1.s0, res1.s1, res1.s2, res1.s3, res1.s4, res1.s5, res1.s6, res1.s7); \
}

DEFINE_MAD_16x16x16_IMPL(bf16, bf16, bf, bf)
DEFINE_MAD_16x16x16_IMPL(fp16, fp16, hf, hf)

INLINE void __builtin_spriv_OpJointMatrixMadINTEL_1x64x16_bf16_bf16_fp32(__private char *a_ptr, __private char *b_ptr, __private char *c_ptr, __private char *d_ptr) {
    short a = *(short *) a_ptr;

    int8 b0 = *(int8 *) b_ptr;
    int8 b1 = *(int8 *)(b_ptr + 1 * 16 * (sizeof (short)));
    int8 b2 = *(int8 *)(b_ptr + 2 * 16 * (sizeof (short)));
    int8 b3 = *(int8 *)(b_ptr + 3 * 16 * (sizeof (short)));

    float c0 = *(float *)  c_ptr;
    float c1 = *(float *) (c_ptr + 1 * (sizeof (int)));
    float c2 = *(float *) (c_ptr + 2 * (sizeof (int)));
    float c3 = *(float *) (c_ptr + 3 * (sizeof (int)));

    float d0 = __builtin_IB_sub_group16_fdpas_f_f_bf_bf_8_1(c0, a, b0);
    float d1 = __builtin_IB_sub_group16_fdpas_f_f_bf_bf_8_1(c1, a, b1);
    float d2 = __builtin_IB_sub_group16_fdpas_f_f_bf_bf_8_1(c2, a, b2);
    float d3 = __builtin_IB_sub_group16_fdpas_f_f_bf_bf_8_1(c3, a, b3);

    __private int4 *dst = (__private int4 *)d_ptr;
    *dst = (int4)(as_int(d0), as_int(d1), as_int(d2), as_int(d3));
}

INLINE void __builtin_spriv_OpJointMatrixMadINTEL_32x64x16_bf16_bf16_fp32(__private char *a_ptr, __private char *b_ptr, __private char *c_ptr, __private char *d_ptr) {
    __private char *a0 = a_ptr;
    __private char *a1 = a_ptr + 16 * (sizeof (short));

    __private char *b0 = b_ptr;
    __private char *b1 = b_ptr + 1 * 16 * (sizeof (short));
    __private char *b2 = b_ptr + 2 * 16 * (sizeof (short));
    __private char *b3 = b_ptr + 3 * 16 * (sizeof (short));

    __private char *c0 = c_ptr + 0 * 16 * (sizeof (int));
    __private char *c1 = c_ptr + 2 * 16 * (sizeof (int));
    __private char *c2 = c_ptr + 4 * 16 * (sizeof (int));
    __private char *c3 = c_ptr + 6 * 16 * (sizeof (int));
    __private char *c4 = c_ptr + 1 * 16 * (sizeof (int));
    __private char *c5 = c_ptr + 3 * 16 * (sizeof (int));
    __private char *c6 = c_ptr + 5 * 16 * (sizeof (int));
    __private char *c7 = c_ptr + 7 * 16 * (sizeof (int));

    __private char *d0 = d_ptr + 0 * 16 * (sizeof (int));
    __private char *d1 = d_ptr + 2 * 16 * (sizeof (int));
    __private char *d2 = d_ptr + 4 * 16 * (sizeof (int));
    __private char *d3 = d_ptr + 6 * 16 * (sizeof (int));
    __private char *d4 = d_ptr + 1 * 16 * (sizeof (int));
    __private char *d5 = d_ptr + 3 * 16 * (sizeof (int));
    __private char *d6 = d_ptr + 5 * 16 * (sizeof (int));
    __private char *d7 = d_ptr + 7 * 16 * (sizeof (int));

    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_bf16_bf16_fp32(a0, b0, c0, d0);
    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_bf16_bf16_fp32(a0, b1, c1, d1);
    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_bf16_bf16_fp32(a0, b2, c2, d2);
    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_bf16_bf16_fp32(a0, b3, c3, d3);

    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_bf16_bf16_fp32(a1, b0, c4, d4);
    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_bf16_bf16_fp32(a1, b1, c5, d5);
    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_bf16_bf16_fp32(a1, b2, c6, d6);
    __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_bf16_bf16_fp32(a1, b3, c7, d7);
}

DEFINE_LOAD(PackedA_RowMajor,     _SG16, short, short, 16, 16, ROW_MAJOR, , 16)
DEFINE_LOAD(PackedA_RowMajor,     _SG16, short, short, 32, 16, ROW_MAJOR, , 32)

DEFINE_LOAD(Accumulator_RowMajor, _SG16, int,   int,   16, 16, ROW_MAJOR, , 16)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int,   int,   32, 16, ROW_MAJOR, , 32)

// sub group size 16
DEFINE_STORE(PackedA_RowMajor,     _SG16, short, short, 16, 16, ROW_MAJOR, , 16, false)
DEFINE_STORE(PackedA_RowMajor,     _SG16, short, short, 32, 16, ROW_MAJOR, , 32, false)
DEFINE_STORE(PackedB_PackedB,      _SG16, short, int,   8, 128, ROW_MAJOR, , 32, false)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int,   int,   16, 16, ROW_MAJOR, , 16, false)

// sub group size 32
DEFINE_STORE(PackedA_RowMajor,     _SG16, short, short, 16, 16, ROW_MAJOR, , 8, false)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int,   int,   16, 16, ROW_MAJOR, , 8, false)

// special case for 1x64 C load: Joint Matrices are expected to be contiguous in memory, without padding at the end of a row
// hence, we can load 1x64 shape using single 2d block load of shape 4x16 instead of 4 1x16 loads
// R_orig is not used
#define DEFINE_LOAD_LARGE_IMPL_(layout, elem_type, elem_bitwidth, contrib_type, R_orig, C, shape, order, WI_rows, WI_rows_per_load, address_space) \
  INLINE void __builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_##address_space##_v8i8_pi32_i32(__private char *dst, char *mem, long stride, int cacheOpt) { \
      long offset = as_long(mem); \
      long baseoffset = offset & (~0x3f); /* align to 64-byte */ \
      int width = sizeof(int) * 16 - 1; /* load 1x64 as 4x16, hence, width is 16 int in bytes */ \
      int height = 4 - 1; /* row count */ \
      int pitch = width; /* JointMatrices are expected to be contiguous in memory, without padding at the end of a row */ \
      long x = (offset - baseoffset) / sizeof(int); /* in elements */ \
      int2 coords = (int2)(x, 0); \
      uint4 __builtin_IB_subgroup_block_read_flat_u32_wi4_m4k16v1(long, int, int, int, int2, int); \
      uint4 res = __builtin_IB_subgroup_block_read_flat_u32_wi4_m4k16v1(baseoffset, width, height, pitch, coords, cacheOpt); \
      *(__private uint4 *)dst = res; \
  }

// _4 in the name is for 4 2d block loads
// R_orig is original number of rows before VNNI
#define DEFINE_LOAD_LARGE_IMPL_4(layout, elem_type, elem_bitwidth, contrib_type, R_orig, C, shape, order, WI_rows, WI_rows_per_load, address_space) \
  INLINE void __builtin_spriv_OpJointMatrixLoadINTEL_##layout##_SG16_##shape##_i##elem_bitwidth##_##WI_rows##_##address_space##_v8i8_pi32_i32(__private char *dst, char *mem, long stride, int cacheOpt) { \
      __private char *dst0 = dst; \
      __private char *dst1 = dst + 1 * WI_rows_per_load * (sizeof (contrib_type)); \
      __private char *dst2 = dst + 2 * WI_rows_per_load * (sizeof (contrib_type)); \
      __private char *dst3 = dst + 3 * WI_rows_per_load * (sizeof (contrib_type)); \
\
      char *mem0 = mem + 0 * MEM_OFFSET_##layout(elem_type, contrib_type); \
      char *mem1 = mem + 1 * MEM_OFFSET_##layout(elem_type, contrib_type); \
      char *mem2 = mem + 2 * MEM_OFFSET_##layout(elem_type, contrib_type); \
      char *mem3 = mem + 3 * MEM_OFFSET_##layout(elem_type, contrib_type); \
\
      __builtin_spriv_OpJointMatrixLoadINTEL_##layout##_SG16_##R_orig##x16_i##elem_bitwidth##_##WI_rows_per_load##_##address_space##_v8i8_pi32_i32(dst0, mem0, stride, cacheOpt); \
      __builtin_spriv_OpJointMatrixLoadINTEL_##layout##_SG16_##R_orig##x16_i##elem_bitwidth##_##WI_rows_per_load##_##address_space##_v8i8_pi32_i32(dst1, mem1, stride, cacheOpt); \
      __builtin_spriv_OpJointMatrixLoadINTEL_##layout##_SG16_##R_orig##x16_i##elem_bitwidth##_##WI_rows_per_load##_##address_space##_v8i8_pi32_i32(dst2, mem2, stride, cacheOpt); \
      __builtin_spriv_OpJointMatrixLoadINTEL_##layout##_SG16_##R_orig##x16_i##elem_bitwidth##_##WI_rows_per_load##_##address_space##_v8i8_pi32_i32(dst3, mem3, stride, cacheOpt); \
  }

#define DEFINE_LOAD_LARGE__(layout, elem_type, elem_bitwidth, contrib_type, contrib_bitwidth, R_orig, C, shape, order, WI_rows, WI_rows_per_load, num_loads) \
  DEFINE_LOAD_LARGE_IMPL_##num_loads(layout, elem_type, elem_bitwidth, contrib_type, R_orig, C, shape, _##order, WI_rows, WI_rows_per_load, generic) \
  DEFINE_LOAD_LARGE_IMPL_##num_loads(layout, elem_type, elem_bitwidth, contrib_type, R_orig, C, shape, _##order, WI_rows, WI_rows_per_load, global ) \
  DEFINE_LOAD_LARGE_IMPL_##num_loads(layout, elem_type, elem_bitwidth, contrib_type, R_orig, C, shape, _##order, WI_rows, WI_rows_per_load, local  )

#define DEFINE_LOAD_LARGE(layout, elem_type, contrib_type, R, C, order, WI_rows, num_loads) \
  DEFINE_LOAD_LARGE__(layout, elem_type, BITWIDTH(elem_type), contrib_type, BITWIDTH(contrib_type), R_ORIG(R, elem_type, contrib_type), C, SHAPE(layout, R, C, elem_type, contrib_type), order, WI_rows, MATH_DIV(WI_rows, num_loads), num_loads)

DEFINE_LOAD_LARGE(PackedB_PackedB,    short, int,  8, 128, ROW_MAJOR,  32, 4)
DEFINE_LOAD_LARGE(PackedB_RowMajor,   short, int,  8, 128, VNNI_TX,    32, 4)
DEFINE_LOAD_LARGE(Accumulator_RowMajor,    ,    ,  1,  64,          ,    ,  )
DEFINE_LOAD_LARGE(Accumulator_RowMajor, int, int, 32,  64, ROW_MAJOR, 128, 4)

#define DEFINE_STORE_ACC_ROW_MAJOR_1x64(address_space) \
  INLINE void __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_##address_space##_pi64_v8i8(char *mem, __private char *src, long stride, int cacheOpt) { \
    long offset = as_long(mem); \
    long baseoffset = offset & (~0x3f); /* align to 64-byte */ \
    int width = sizeof(int) * 16 - 1; /* in bytes, load 1x64 as 4x16 to use one load instead of 4 */ \
    int pitch = width; /* JointMatrices are expected to be contiguous in memory, without padding at the end of a row */ \
    int height = 4 - 1; /* row count */ \
    long x = (offset - baseoffset) / sizeof(int); /* in elements */ \
    int2 coords = (int2)(x, 0); \
    uint4 val = *(uint4 *)src; \
    void __builtin_IB_subgroup_block_write_flat_u32_wi4_m4k16v1(long, int, int, int, int2, uint4, int); \
    __builtin_IB_subgroup_block_write_flat_u32_wi4_m4k16v1(baseoffset, width, height, pitch, coords, val, cacheOpt); \
  }

#define DEFINE_STORE_ACC_ROW_MAJOR_32x64(address_space) \
  INLINE void __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_##address_space##_pi64_v8i8(char *mem, __private char *src, long stride, int cacheOpt) { \
      __private char *c0 = src + 0 * 16 * (sizeof (int)); \
      __private char *c1 = src + 1 * 16 * (sizeof (int)); \
      __private char *c2 = src + 2 * 16 * (sizeof (int)); \
      __private char *c3 = src + 3 * 16 * (sizeof (int)); \
      __private char *c4 = src + 4 * 16 * (sizeof (int)); \
      __private char *c5 = src + 5 * 16 * (sizeof (int)); \
      __private char *c6 = src + 6 * 16 * (sizeof (int)); \
      __private char *c7 = src + 7 * 16 * (sizeof (int)); \
\
      char *mem0 = mem + 0 * 16 * (sizeof (int)); \
      char *mem1 = mem + 0 * 16 * (sizeof (int)) + 16 * (sizeof (int)) * stride; \
      char *mem2 = mem + 1 * 16 * (sizeof (int)); \
      char *mem3 = mem + 1 * 16 * (sizeof (int)) + 16 * (sizeof (int)) * stride; \
      char *mem4 = mem + 2 * 16 * (sizeof (int)); \
      char *mem5 = mem + 2 * 16 * (sizeof (int)) + 16 * (sizeof (int)) * stride; \
      char *mem6 = mem + 3 * 16 * (sizeof (int)); \
      char *mem7 = mem + 3 * 16 * (sizeof (int)) + 16 * (sizeof (int)) * stride; \
\
      __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_##address_space##_pi64_v8i8(mem0, c0, stride, cacheOpt); \
      __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_##address_space##_pi64_v8i8(mem1, c1, stride, cacheOpt); \
      __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_##address_space##_pi64_v8i8(mem2, c2, stride, cacheOpt); \
      __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_##address_space##_pi64_v8i8(mem3, c3, stride, cacheOpt); \
      __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_##address_space##_pi64_v8i8(mem4, c4, stride, cacheOpt); \
      __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_##address_space##_pi64_v8i8(mem5, c5, stride, cacheOpt); \
      __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_##address_space##_pi64_v8i8(mem6, c6, stride, cacheOpt); \
      __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_##address_space##_pi64_v8i8(mem7, c7, stride, cacheOpt); \
  }

#define DEFINE_STORE_ACC_ROW_MAJOR_LARGE(R, C) \
  DEFINE_STORE_ACC_ROW_MAJOR_##R##x##C(generic) \
  DEFINE_STORE_ACC_ROW_MAJOR_##R##x##C(global) \
  DEFINE_STORE_ACC_ROW_MAJOR_##R##x##C(local)

DEFINE_STORE_ACC_ROW_MAJOR_LARGE( 1, 64)
DEFINE_STORE_ACC_ROW_MAJOR_LARGE(32, 64)
