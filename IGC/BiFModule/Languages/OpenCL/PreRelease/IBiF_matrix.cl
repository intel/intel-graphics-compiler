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
extern __constant int __JointMatrixLoadStoreOpt;

// Matrix order
#define _ROW_MAJOR 0
#define _COL_MAJOR 1
#define _VNNI_TX   2

// Address space
#define AS_GENERIC 0
#define AS_LOCAL   1
#define AS_GLOBAL  2

#define TF32 1
#define notTF32 0

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

#define IND_ROW_MAJOR(slid, stride, i) (slid + (i * stride))
#define IND_COL_MAJOR(slid, stride, i) (i + (slid * stride))
#define IND_VNNI_TX(slid, stride, i) (i + (slid * stride))

// The following variations are added to calculate index based on the new
// slicing idea for MatA of TF32 type.  Currently we do not have any test case
// of col_major A of TF32 type, therefore we keep the implementation of
// COL_MAJOR same as the old slicing case.
// Arguments:
//   sg_cols: Number of columns held in the subgroup
//   skip_factor: n, where we include elements from every n-th row of the JM
//   to be part of the wi.  e.g for a Matrix
//     1 2 3 4
//     5 6 7 8
//     9 10 11 12
//     13 14 15 16
//    if skip_factor == 2, we will include items <1, 9> (every "2"nd row) in the
//    first WI, <2, 10> in the second WI and so on..
#define IND_ROW_MAJOR_TF32_NEW_SLIC(slid, stride, skip_factor, i, sg_cols) ((slid/sg_cols*stride) + (i*stride*skip_factor) + (slid%sg_cols))
#define IND_COL_MAJOR_TF32_NEW_SLIC(slid, stride, skip_factor, i, sg_cols) (i + (slid * stride)) // placeholder
#define IND_VNNI_TX_TF32_NEW_SLIC(slid, stride, skip_factor, i, sg_cols) (i + (slid * stride)) // placeholder

// no int7, int6, int5 types
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

#define OUT_VEC16(type) type##16
#define OUT_VEC8(type) type##8
#define OUT_VEC7(type) type##8
#define OUT_VEC6(type) type##8
#define OUT_VEC5(type) type##8
#define OUT_VEC4(type) type##4
#define OUT_VEC3(type) type##3
#define OUT_VEC2(type) type##2
#define OUT_VEC1(type) type

#define OUT_STORE_VEC8(type) type##8
#define OUT_STORE_VEC7(type) type##8
#define OUT_STORE_VEC6(type) type##8
#define OUT_STORE_VEC5(type) type##8
#define OUT_STORE_VEC4(type) type##4
#define OUT_STORE_VEC3(type) type##4
#define OUT_STORE_VEC2(type) type##2
#define OUT_STORE_VEC1(type) type

// layout can be PackedA_RowMajor, PackedB_ColumnMajor, PackedB_PackedB, etc.
// sg is empty for XMX8 and _SG16 for PVC
// elem_bitwidth is 8, 16 or 32
// shape is shape of the matrix, like 8x16
#define MANGLE_LOAD_NAME_AS_GENERIC(layout, sg, elem_bitwidth, shape) \
  __builtin_spriv_OpJointMatrixLoadINTEL_##layout##sg##_##shape##_i##elem_bitwidth##_generic_v8i8_pi32_i32

#define MANGLE_LOAD_NAME_AS_LOCAL(layout, sg, elem_bitwidth, shape) \
  __builtin_spriv_OpJointMatrixLoadINTEL_##layout##sg##_##shape##_i##elem_bitwidth##_local_v8i8_pi32_i32

#define MANGLE_LOAD_NAME_AS_GLOBAL(layout, sg, elem_bitwidth, shape) \
  __builtin_spriv_OpJointMatrixLoadINTEL_##layout##sg##_##shape##_i##elem_bitwidth##_global_v8i8_pi32_i32

#define SUB_GROUP_LOAD(readop, M, src, dst, stride, contrib_type) \
    contrib_type *ptr = (contrib_type *)mem; \
    __private contrib_type *wi_contrib = (__private contrib_type *)dst; \
    for (int i = 0; i < M; i++) \
        wi_contrib[i] = readop((src) + i * (stride));

// variants for 7,6,5,3 and 1 are only used to make the code compilable
#define DEFINE_BLOCK_RW_NAME8(rw, us) intel_sub_group_block_##rw##us##8
#define DEFINE_BLOCK_RW_NAME7(rw, us) intel_sub_group_block_##rw##us##8
#define DEFINE_BLOCK_RW_NAME6(rw, us) intel_sub_group_block_##rw##us##8
#define DEFINE_BLOCK_RW_NAME5(rw, us) intel_sub_group_block_##rw##us##8
#define DEFINE_BLOCK_RW_NAME4(rw, us) intel_sub_group_block_##rw##us##4
#define DEFINE_BLOCK_RW_NAME3(rw, us) intel_sub_group_block_##rw##us##4
#define DEFINE_BLOCK_RW_NAME2(rw, us) intel_sub_group_block_##rw##us##2
#define DEFINE_BLOCK_RW_NAME1(rw, us) intel_sub_group_block_##rw##us

#define DEFINE_BLOCK2D_RW_NAME(rw, tx, contrib_bitwidth, M, K) __builtin_IB_subgroup_block_##rw##_flat##tx##_u##contrib_bitwidth##_m##M##k##K##v1
#define DEFINE_BLOCK2D_VNNI_NAME(contrib_bitwidth, K) __builtin_IB_subgroup_block_read_flat_transform_u##contrib_bitwidth##_k##K

/* For platforms without SG16 JointMatrix support block2d is not available. The
 * implementation remains empty, will fallthrough to vector implementation. */
#define IMPLEMENT_BLOCK2D_LOAD_ROW_MAJOR(element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, stride_opt) \
  /* not supported, fallthrough */
#define IMPLEMENT_BLOCK2D_LOAD_COL_MAJOR(element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, stride_opt) \
  /* not supported, fallthrough */
#define IMPLEMENT_BLOCK2D_LOAD_VNNI_TX(element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, stride_opt) \
  /* not supported, fallthrough */
#define IMPLEMENT_BLOCK2D_STORE(element_type, contrib_type, contrib_bitwidth, M, K, vec) \
  /* not supported, fallthrough */

#define IMPLEMENT_BLOCK2D_LOAD_SG16_ROW_MAJOR(element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, stride_opt) \
    long offset = as_long(mem); \
    long baseoffset = offset & (~0x3f); /* align to 64-byte */ \
    int width = (sizeof (element_type)) * stride - 1; /* in bytes */ \
    int pitch = width; /* JointMatrices are expected to be contigunous in memory, without padding at the end of a row */ \
    int height = M - 1; /* row count */ \
    long x = (offset - baseoffset) / (sizeof (contrib_type)); /* in elements */ \
    int2 coords = (int2)(x, 0); \
    OUT_VEC##M(u##contrib_type) DEFINE_BLOCK2D_RW_NAME(read, , contrib_bitwidth, M, K)(long, int, int, int, int2); \
    OUT_VEC##M(u##contrib_type) res = DEFINE_BLOCK2D_RW_NAME(read, , contrib_bitwidth, M, K)(baseoffset, width, height, pitch, coords); \
    *(__private OUT_VEC##M(u##contrib_type) *)dst = res; \
    return;

#define IMPLEMENT_BLOCK2D_LOAD_SG16_COL_MAJOR(element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, stride_opt) \
    long offset = as_long(mem); \
    long baseoffset = offset & (~0x3f); /* align to 64-byte */ \
    int width = (sizeof (element_type)) * stride - 1; /* in bytes */ \
    int pitch = width; /* JointMatrices are expected to be contigunous in memory, without padding at the end of a row */ \
    int height = M - 1; /* row count */ \
    long x = (offset - baseoffset) / (sizeof (contrib_type)); /* in elements */ \
    int2 coords = (int2)(x, 0); \
    OUT_VEC##M(u##contrib_type) DEFINE_BLOCK2D_RW_NAME(read, _transpose, elem_bitwidth, M, K)(long, int, int, int, int2); \
    OUT_VEC##M(u##contrib_type) res = DEFINE_BLOCK2D_RW_NAME(read, _transpose, elem_bitwidth, M, K)(baseoffset, width, height, pitch, coords); \
    *(__private OUT_VEC##M(u##contrib_type) *)dst = res; \
    return;

#define IMPLEMENT_BLOCK2D_LOAD_SG16_VNNI_TX(element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, stride_opt) \
    long offset = as_long(mem); \
    long baseoffset = offset & (~0x3f); /* align to 64-byte */ \
    int width = (sizeof (element_type)) * stride - 1; /* in bytes */ \
    int pitch = width; /* JointMatrices are expected to be contigunous in memory, without padding at the end of a row */ \
    int height = stride_opt - 1; /* row count */ \
    long x = (offset - baseoffset) / (sizeof (element_type)); /* in elements */ \
    int2 coords = (int2)(x, 0); \
    OUT_VEC##M(u##contrib_type) DEFINE_BLOCK2D_VNNI_NAME(elem_bitwidth, K)(long, int, int, int, int2); \
    OUT_VEC##M(u##contrib_type) res = DEFINE_BLOCK2D_VNNI_NAME(elem_bitwidth, K)(baseoffset, width, height, pitch, coords); \
    *(__private OUT_VEC##M(u##contrib_type) *)dst = res; \
    return;

#define IMPLEMENT_BLOCK2D_STORE_SG16(element_type, contrib_type, contrib_bitwidth, M, K, vec) \
    long offset = as_long(mem); \
    long baseoffset = offset & (~0x3f); /* align to 64-byte */ \
    int width = (sizeof (element_type)) * stride - 1; /* in bytes */ \
    int pitch = width; /* JointMatrices are expected to be contigunous in memory, without padding at the end of a row */ \
    int height = M - 1; /* row count */ \
    long x = (offset - baseoffset) / (sizeof (contrib_type)); /* in elements */ \
    int2 coords = (int2)(x, 0); \
    void DEFINE_BLOCK2D_RW_NAME(write, , contrib_bitwidth, M, K)(long, int, int, int, int2, OUT_VEC##M(u##contrib_type)); \
    OUT_VEC##M(u##contrib_type) val = *(OUT_VEC##M(u##contrib_type) *)src; \
    DEFINE_BLOCK2D_RW_NAME(write, , contrib_bitwidth, M, K)(baseoffset, width, height, pitch, coords, val); \
    return;

// layout can be PackedA_RowMajor, PackedB_ColumnMajor, PackedB_PackedB, etc.
// sg is empty for XMX8 and _SG16 for PVC
// element_type is char for i8, short for i16 and int for i32
// elem_bitwidth is the bitwidth of the elem_type, expected values are 8, 16 or 32
// contrib_type is int or short depending on available OpenCL extension API
// contrib_bitwidth is the bitwidth of the contrib_type, expected values are 8, 16 or 32
// M is number of rows
// K is number of columns
// shape is shape of the matrix, like 8x16. We can not replace shape with M and stride_opt parameters,
//      in case of vnni'd B, so we keep it.
// order is ROW_MAJOR or COL_MAJOR
// us is empty for int contrib type and _us for short contrib type.
// stride_opt should be either equal to C or 2*C in case of matrix B, since matrix B is vnni'ed
// Currently, for TF32 type, we use naive load/store
#define DEFINE_LOAD_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, stride_opt, address_space, isTF32) \
  INLINE void MANGLE_LOAD_NAME_##address_space(layout, sg, elem_bitwidth, shape) (__private char *dst, char *mem, long stride) { \
      if (!isTF32 && __JointMatrixLoadStoreOpt >= BLOCK2D_IMPL && (M == 2 || M == 4 || M == 8) \
          && (order == _ROW_MAJOR || order == _VNNI_TX) && address_space == AS_GLOBAL \
          ) { \
          IMPLEMENT_BLOCK2D_LOAD##sg##order(element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, stride_opt) \
      } \
      if (!isTF32 && __JointMatrixLoadStoreOpt >= VECTOR_CONT_IMPL \
          && stride == stride_opt && (M == 2 || M == 4 || M == 8) && order == _ROW_MAJOR \
          && (address_space == AS_GLOBAL || address_space == AS_LOCAL) \
          ) { \
          OUT_STORE_VEC##M(u##contrib_type) res = DEFINE_BLOCK_RW_NAME##M(read, us)((ATTRIBUTE_##address_space u##contrib_type *)mem); \
          *(__private OUT_VEC##M(u##contrib_type) *)dst = *(__private OUT_VEC##M(u##contrib_type) *)&res; \
          return; \
      } \
      if (!isTF32 && __JointMatrixLoadStoreOpt >= VECTOR_IMPL && order == _ROW_MAJOR \
          && (address_space == AS_GLOBAL || address_space == AS_LOCAL) \
          ) { \
          int pack_factor = sizeof (u##contrib_type) / sizeof (element_type); \
          stride = stride / pack_factor; \
          SUB_GROUP_LOAD(intel_sub_group_block_read##us, M, (ATTRIBUTE_##address_space u##contrib_type *)mem, dst, stride, contrib_type) \
          return; \
      } \
      contrib_type *ptr = (contrib_type *)mem; \
      int slid = get_sub_group_local_id(); \
      int pack_factor = sizeof (contrib_type) / sizeof (element_type); \
      stride = stride / pack_factor; \
      __private contrib_type *wi_contrib = (__private contrib_type *)dst; \
      for (int i = 0; i < M; i++) { \
          if (isTF32 && (_##layout == _PackedA_RowMajor)) { \
            int skip_factor = 2; /*num_rows_sg / num_rows_wi*/ \
            wi_contrib[i] = ptr[IND##order##_TF32_NEW_SLIC(slid, stride, skip_factor, i, K /*sg_cols*/)]; \
          } else { \
            wi_contrib[i] = ptr[IND##order(slid, stride, i)]; \
          } \
      } \
  }

#define DEFINE_LOAD(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, stride_opt, isTF32) \
  DEFINE_LOAD_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, stride_opt, AS_GENERIC, isTF32) \
  DEFINE_LOAD_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, stride_opt, AS_LOCAL, isTF32) \
  DEFINE_LOAD_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, stride_opt, AS_GLOBAL, isTF32)

/* PackedA load i16 */
DEFINE_LOAD(PackedA_RowMajor, , short, 16, int, 32, 8, 16, 8x16, ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(PackedA_RowMajor, , short, 16, int, 32, 7, 16, 7x16, ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(PackedA_RowMajor, , short, 16, int, 32, 6, 16, 6x16, ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(PackedA_RowMajor, , short, 16, int, 32, 5, 16, 5x16, ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(PackedA_RowMajor, , short, 16, int, 32, 4, 16, 4x16, ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(PackedA_RowMajor, , short, 16, int, 32, 3, 16, 3x16, ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(PackedA_RowMajor, , short, 16, int, 32, 2, 16, 2x16, ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(PackedA_RowMajor, , short, 16, int, 32, 1, 16, 1x16, ROW_MAJOR, , 16, notTF32)

/* PackedA load i8 */
DEFINE_LOAD(PackedA_RowMajor, , char, 8, int, 32, 8, 32, 8x32, ROW_MAJOR, , 32, notTF32)
DEFINE_LOAD(PackedA_RowMajor, , char, 8, int, 32, 7, 32, 7x32, ROW_MAJOR, , 32, notTF32)
DEFINE_LOAD(PackedA_RowMajor, , char, 8, int, 32, 6, 32, 6x32, ROW_MAJOR, , 32, notTF32)
DEFINE_LOAD(PackedA_RowMajor, , char, 8, int, 32, 5, 32, 5x32, ROW_MAJOR, , 32, notTF32)
DEFINE_LOAD(PackedA_RowMajor, , char, 8, int, 32, 4, 32, 4x32, ROW_MAJOR, , 32, notTF32)
DEFINE_LOAD(PackedA_RowMajor, , char, 8, int, 32, 3, 32, 3x32, ROW_MAJOR, , 32, notTF32)
DEFINE_LOAD(PackedA_RowMajor, , char, 8, int, 32, 2, 32, 2x32, ROW_MAJOR, , 32, notTF32)
DEFINE_LOAD(PackedA_RowMajor, , char, 8, int, 32, 1, 32, 1x32, ROW_MAJOR, , 32, notTF32)

/* PackedA load i16 SG16 */
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, 16, short, 16, 8, 16, 8x16, ROW_MAJOR, _us, 16, notTF32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, 16, short, 16, 7, 16, 7x16, ROW_MAJOR, _us, 16, notTF32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, 16, short, 16, 6, 16, 6x16, ROW_MAJOR, _us, 16, notTF32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, 16, short, 16, 5, 16, 5x16, ROW_MAJOR, _us, 16, notTF32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, 16, short, 16, 4, 16, 4x16, ROW_MAJOR, _us, 16, notTF32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, 16, short, 16, 3, 16, 3x16, ROW_MAJOR, _us, 16, notTF32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, 16, short, 16, 2, 16, 2x16, ROW_MAJOR, _us, 16, notTF32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, 16, short, 16, 1, 16, 1x16, ROW_MAJOR, _us, 16, notTF32)

/* PackedA load i8 SG16 */
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, 8, short, 16, 8, 16, 8x32, ROW_MAJOR, _us, 32, notTF32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, 8, short, 16, 7, 16, 7x32, ROW_MAJOR, _us, 32, notTF32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, 8, short, 16, 6, 16, 6x32, ROW_MAJOR, _us, 32, notTF32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, 8, short, 16, 5, 16, 5x32, ROW_MAJOR, _us, 32, notTF32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, 8, short, 16, 4, 16, 4x32, ROW_MAJOR, _us, 32, notTF32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, 8, short, 16, 3, 16, 3x32, ROW_MAJOR, _us, 32, notTF32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, 8, short, 16, 2, 16, 2x32, ROW_MAJOR, _us, 32, notTF32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, 8, short, 16, 1, 16, 1x32, ROW_MAJOR, _us, 32, notTF32)

/* A load tf32 SG16 */
// Please note that even though the JM size is 8x8, we are passing 4 as the parameter M
// instead of 8, because due to new slicing, we want each WI to return a vector of 4
// elements.  Currently, we use M as the size of the vector.  Making a change to use
// M/2 will require singinifcant code changes, therefore we are using M=4 here.
DEFINE_LOAD(PackedA_RowMajor, _SG16, int, 32, int, 32, 4, 8, 8x8, ROW_MAJOR, , 8, TF32)

/* PackedB load i16 */
DEFINE_LOAD(PackedB_ColumnMajor, , short, 16, int, 32, 8, 8,  16x8,  COL_MAJOR, , -1, notTF32)
DEFINE_LOAD(PackedB_PackedB,     , short, 16, int, 32, 8, 8,  16x8,  ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(PackedB_PackedB,     , short, 16, int, 32, 8, 16, 16x16, ROW_MAJOR, , 32, notTF32)

/* PackedB load i8 */
DEFINE_LOAD(PackedB_ColumnMajor, , char, 8, int, 32, 8, 8,  32x8,  COL_MAJOR, , -1, notTF32)
DEFINE_LOAD(PackedB_PackedB,     , char, 8, int, 32, 8, 8,  32x8,  ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(PackedB_PackedB,     , char, 8, int, 32, 8, 16, 32x16, ROW_MAJOR, , 32, notTF32)

/* PackedB load i16 SG16 */
DEFINE_LOAD(PackedB_ColumnMajor, _SG16, short, 16, int, 32, 8, 8,  16x8,  COL_MAJOR, , -1, notTF32)
DEFINE_LOAD(PackedB_PackedB,     _SG16, short, 16, int, 32, 8, 8,  16x8,  ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(PackedB_PackedB,     _SG16, short, 16, int, 32, 8, 16, 16x16, ROW_MAJOR, , 32, notTF32)
DEFINE_LOAD(PackedB_RowMajor,    _SG16, short, 16, int, 32, 8, 8,  16x8,  VNNI_TX, , 16, notTF32)
DEFINE_LOAD(PackedB_RowMajor,    _SG16, short, 16, int, 32, 8, 16, 16x16, VNNI_TX, , 16, notTF32)

/* PackedB load i8 SG16*/
DEFINE_LOAD(PackedB_ColumnMajor, _SG16, char, 8, int, 32, 8, 8,  32x8,  COL_MAJOR, , -1, notTF32)
DEFINE_LOAD(PackedB_PackedB,     _SG16, char, 8, int, 32, 8, 8,  32x8,  ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(PackedB_PackedB,     _SG16, char, 8, int, 32, 8, 16, 32x16, ROW_MAJOR, , 32, notTF32)
DEFINE_LOAD(PackedB_RowMajor,    _SG16, char, 8, int, 32, 8, 8,  32x8,  VNNI_TX, , 16, notTF32)
DEFINE_LOAD(PackedB_RowMajor,    _SG16, char, 8, int, 32, 8, 16, 32x16, VNNI_TX, , 32, notTF32)

/* B load tf32 SG16 */
DEFINE_LOAD(PackedB_RowMajor, _SG16, int, 32, int, 32, 8, 16,  8x16,  ROW_MAJOR, , 16, TF32)

/* Load accumulator is a special case of load packed A, both are row major: */
DEFINE_LOAD(Accumulator_RowMajor, , int, 32, int, 32, 8, 8, 8x8, ROW_MAJOR, , 8, notTF32)
DEFINE_LOAD(Accumulator_RowMajor, , int, 32, int, 32, 7, 8, 7x8, ROW_MAJOR, , 8, notTF32)
DEFINE_LOAD(Accumulator_RowMajor, , int, 32, int, 32, 6, 8, 6x8, ROW_MAJOR, , 8, notTF32)
DEFINE_LOAD(Accumulator_RowMajor, , int, 32, int, 32, 5, 8, 5x8, ROW_MAJOR, , 8, notTF32)
DEFINE_LOAD(Accumulator_RowMajor, , int, 32, int, 32, 4, 8, 4x8, ROW_MAJOR, , 8, notTF32)
DEFINE_LOAD(Accumulator_RowMajor, , int, 32, int, 32, 3, 8, 3x8, ROW_MAJOR, , 8, notTF32)
DEFINE_LOAD(Accumulator_RowMajor, , int, 32, int, 32, 2, 8, 2x8, ROW_MAJOR, , 8, notTF32)
DEFINE_LOAD(Accumulator_RowMajor, , int, 32, int, 32, 1, 8, 1x8, ROW_MAJOR, , 8, notTF32)

DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, 32, int, 32, 8, 16, 8x16, ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, 32, int, 32, 7, 16, 7x16, ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, 32, int, 32, 6, 16, 6x16, ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, 32, int, 32, 5, 16, 5x16, ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, 32, int, 32, 4, 16, 4x16, ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, 32, int, 32, 3, 16, 3x16, ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, 32, int, 32, 2, 16, 2x16, ROW_MAJOR, , 16, notTF32)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, 32, int, 32, 1, 16, 1x16, ROW_MAJOR, , 16, notTF32)

// --------- STORE built-ins --------------------------------------

#define MANGLE_STORE_NAME_AS_GENERIC(layout, sg, elem_bitwidth, shape) \
  __builtin_spriv_OpJointMatrixStoreINTEL_##layout##sg##_##shape##_i##elem_bitwidth##_generic_pi64_v8i8
#define MANGLE_STORE_NAME_AS_LOCAL(layout, sg, elem_bitwidth, shape) \
  __builtin_spriv_OpJointMatrixStoreINTEL_##layout##sg##_##shape##_i##elem_bitwidth##_local__pi64_v8i8
#define MANGLE_STORE_NAME_AS_GLOBAL(layout, sg, elem_bitwidth, shape) \
  __builtin_spriv_OpJointMatrixStoreINTEL_##layout##sg##_##shape##_i##elem_bitwidth##_global_pi64_v8i8

#define VEC_IND8(var, ind) var[ind]
#define VEC_IND7(var, ind) var[ind]
#define VEC_IND6(var, ind) var[ind]
#define VEC_IND5(var, ind) var[ind]
#define VEC_IND4(var, ind) var[ind]
#define VEC_IND3(var, ind) var[ind]
#define VEC_IND2(var, ind) var[ind]
#define VEC_IND1(var, ind) var

// set block_opt to false to disable block non-continous optimization per one built-in as a workaround
#define DEFINE_STORE_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, stride_opt, block_opt, address_space, isTF32) \
  INLINE void MANGLE_STORE_NAME_##address_space(layout, sg, elem_bitwidth, shape) (char *mem, __private char *src, long stride) { \
      if (!isTF32 && __JointMatrixLoadStoreOpt >= BLOCK2D_IMPL && (M == 2 || M == 4 || M == 8) \
          && order == _ROW_MAJOR && address_space == AS_GLOBAL && elem_bitwidth > 8 \
          ) { \
          IMPLEMENT_BLOCK2D_STORE##sg(element_type, contrib_type, contrib_bitwidth, M, K, src) \
      } \
      if (!isTF32 && __JointMatrixLoadStoreOpt >= VECTOR_CONT_IMPL && stride == stride_opt \
          && (M == 2 || M == 4 || M == 8) && order == _ROW_MAJOR \
          && (address_space == AS_GLOBAL || address_space == AS_LOCAL) \
          ) { \
          OUT_VEC##M(contrib_type) vec = *(__private OUT_VEC##M(contrib_type) *)src; \
          DEFINE_BLOCK_RW_NAME##M(write, us)((ATTRIBUTE_##address_space u##contrib_type *)mem, VEC_TO_VEC_STORE##M(u##contrib_type , vec)); \
          return; \
      } \
      if (!isTF32 && (__JointMatrixLoadStoreOpt >= VECTOR_IMPL) \
          && order == _ROW_MAJOR && block_opt == true \
          && (address_space == AS_GLOBAL || address_space == AS_LOCAL) \
          ) { \
          ATTRIBUTE_##address_space u##contrib_type *ptr = (ATTRIBUTE_##address_space u##contrib_type *)mem; \
          int pack_factor = sizeof (u##contrib_type) / sizeof (element_type); \
          stride = stride / pack_factor; \
          for (int i = 0; i < M; i++) \
              intel_sub_group_block_write##us(ptr + i * stride, ((__private u##contrib_type *)src)[i]); \
          return; \
      } \
      contrib_type *ptr = (contrib_type *)mem; \
      int slid = get_sub_group_local_id(); \
      int pack_factor = sizeof (contrib_type) / sizeof (element_type); \
      stride = stride / pack_factor; \
      __private contrib_type *slice = (__private contrib_type *)src; \
      for (int i = 0; i < M; i++) { \
        if (isTF32 && (_##layout == _PackedA_RowMajor)) { \
            ptr[ IND##order##_TF32_NEW_SLIC(slid, stride, 2 /*skip_factor*/, i, K /*sg_cols*/) ] = slice[i]; \
          } else { \
            ptr[ IND##order(slid, stride, i) ] = slice[i]; \
          } \
      } \
  }

#define DEFINE_STORE(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, order, us, stride_opt, block_opt, isTF32) \
  DEFINE_STORE_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, stride_opt, block_opt, AS_GENERIC, isTF32) \
  DEFINE_STORE_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, stride_opt, block_opt, AS_LOCAL, isTF32) \
  DEFINE_STORE_IMPL(layout, sg, element_type, elem_bitwidth, contrib_type, contrib_bitwidth, M, K, shape, _##order, us, stride_opt, block_opt, AS_GLOBAL, isTF32)

// TODO: investigate why intel_sub_group_block_write causes an assertion and enable blocked non-continuous optimization

/* PackedA store i8 */
DEFINE_STORE(PackedA_RowMajor,      , char,  8,  int,   32, 1, 32, 1x32, ROW_MAJOR,    , 32, false, notTF32)
DEFINE_STORE(PackedA_RowMajor,      , char,  8,  int,   32, 2, 32, 2x32, ROW_MAJOR,    , 32, false, notTF32)
DEFINE_STORE(PackedA_RowMajor,      , char,  8,  int,   32, 3, 32, 3x32, ROW_MAJOR,    , 32, false, notTF32)
DEFINE_STORE(PackedA_RowMajor,      , char,  8,  int,   32, 4, 32, 4x32, ROW_MAJOR,    , 32, false, notTF32)
DEFINE_STORE(PackedA_RowMajor,      , char,  8,  int,   32, 5, 32, 5x32, ROW_MAJOR,    , 32, false, notTF32)
DEFINE_STORE(PackedA_RowMajor,      , char,  8,  int,   32, 6, 32, 6x32, ROW_MAJOR,    , 32, false, notTF32)
DEFINE_STORE(PackedA_RowMajor,      , char,  8,  int,   32, 7, 32, 7x32, ROW_MAJOR,    , 32, false, notTF32)
DEFINE_STORE(PackedA_RowMajor,      , char,  8,  int,   32, 8, 32, 8x32, ROW_MAJOR,    , 32, false, notTF32)

/* PackedA store i16 */
DEFINE_STORE(PackedA_RowMajor,      , short, 16, int,   32, 1, 16, 1x16, ROW_MAJOR,    , 16, false, notTF32)
DEFINE_STORE(PackedA_RowMajor,      , short, 16, int,   32, 2, 16, 2x16, ROW_MAJOR,    , 16, false, notTF32)
DEFINE_STORE(PackedA_RowMajor,      , short, 16, int,   32, 3, 16, 3x16, ROW_MAJOR,    , 16, false, notTF32)
DEFINE_STORE(PackedA_RowMajor,      , short, 16, int,   32, 4, 16, 4x16, ROW_MAJOR,    , 16, false, notTF32)
DEFINE_STORE(PackedA_RowMajor,      , short, 16, int,   32, 5, 16, 5x16, ROW_MAJOR,    , 16, false, notTF32)
DEFINE_STORE(PackedA_RowMajor,      , short, 16, int,   32, 6, 16, 6x16, ROW_MAJOR,    , 16, false, notTF32)
DEFINE_STORE(PackedA_RowMajor,      , short, 16, int,   32, 7, 16, 7x16, ROW_MAJOR,    , 16, false, notTF32)
DEFINE_STORE(PackedA_RowMajor,      , short, 16, int,   32, 8, 16, 8x16, ROW_MAJOR,    , 16, false, notTF32)

/* PackedA store i8 SG16 */
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  8,  short, 16, 1, 32, 1x32, ROW_MAJOR, _us, 32, false, notTF32)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  8,  short, 16, 2, 32, 2x32, ROW_MAJOR, _us, 32, false, notTF32)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  8,  short, 16, 3, 32, 3x32, ROW_MAJOR, _us, 32, false, notTF32)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  8,  short, 16, 4, 32, 4x32, ROW_MAJOR, _us, 32, false, notTF32)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  8,  short, 16, 5, 32, 5x32, ROW_MAJOR, _us, 32, false, notTF32)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  8,  short, 16, 6, 32, 6x32, ROW_MAJOR, _us, 32, false, notTF32)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  8,  short, 16, 7, 32, 7x32, ROW_MAJOR, _us, 32, false, notTF32)
DEFINE_STORE(PackedA_RowMajor, _SG16, char,  8,  short, 16, 8, 32, 8x32, ROW_MAJOR, _us, 32, false, notTF32)

/* PackedA store i16 SG16 */
DEFINE_STORE(PackedA_RowMajor, _SG16, short, 16, short, 16, 1, 16, 1x16, ROW_MAJOR, _us, 16, false, notTF32)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, 16, short, 16, 2, 16, 2x16, ROW_MAJOR, _us, 16, false, notTF32)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, 16, short, 16, 3, 16, 3x16, ROW_MAJOR, _us, 16, false, notTF32)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, 16, short, 16, 4, 16, 4x16, ROW_MAJOR, _us, 16, false, notTF32)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, 16, short, 16, 5, 16, 5x16, ROW_MAJOR, _us, 16, false, notTF32)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, 16, short, 16, 6, 16, 6x16, ROW_MAJOR, _us, 16, false, notTF32)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, 16, short, 16, 7, 16, 7x16, ROW_MAJOR, _us, 16, false, notTF32)
DEFINE_STORE(PackedA_RowMajor, _SG16, short, 16, short, 16, 8, 16, 8x16, ROW_MAJOR, _us, 16, false, notTF32)

/* A store tf32 SG16 */
DEFINE_STORE(PackedA_RowMajor, _SG16, int, 32, int, 32, 4, 8, 8x8, ROW_MAJOR, , 8, false, TF32)

DEFINE_STORE(PackedB_PackedB,      , short, 16, int, 32, 8, 16, 16x8,  ROW_MAJOR, , 16, true, notTF32)
DEFINE_STORE(PackedB_PackedB,      , short, 16, int, 32, 8, 16, 16x16, ROW_MAJOR, , 32, true, notTF32)
DEFINE_STORE(PackedB_PackedB, _SG16, short, 16, int, 32, 8, 16, 16x8,  ROW_MAJOR, , 16, true, notTF32)
DEFINE_STORE(PackedB_PackedB, _SG16, short, 16, int, 32, 8, 16, 16x16, ROW_MAJOR, , 32, true, notTF32)

// TODO: investigate why intel_sub_group_block_write causes an assertion and enable blocked non-continuous optimization
DEFINE_STORE(PackedB_PackedB,      , char, 8, int, 32, 8, 32, 32x8, ROW_MAJOR, , 16, false, notTF32)
DEFINE_STORE(PackedB_PackedB, _SG16, char, 8, int, 32, 8, 32, 32x8, ROW_MAJOR, , 16, false, notTF32)
DEFINE_STORE(PackedB_PackedB, _SG16, char, 8, int, 32, 8, 32, 32x16, ROW_MAJOR, , 32, false, notTF32)

/* B store tf32 SG16 */
DEFINE_STORE(PackedB_RowMajor, _SG16, int, 32, int, 32, 8, 16, 8x16, ROW_MAJOR, , 16, true, TF32)

DEFINE_STORE(Accumulator_RowMajor, , int, 32, int, 32, 8, 8, 8x8, ROW_MAJOR, , 8, true, notTF32)
DEFINE_STORE(Accumulator_RowMajor, , int, 32, int, 32, 7, 8, 7x8, ROW_MAJOR, , 8, true, notTF32)
DEFINE_STORE(Accumulator_RowMajor, , int, 32, int, 32, 6, 8, 6x8, ROW_MAJOR, , 8, true, notTF32)
DEFINE_STORE(Accumulator_RowMajor, , int, 32, int, 32, 5, 8, 5x8, ROW_MAJOR, , 8, true, notTF32)
DEFINE_STORE(Accumulator_RowMajor, , int, 32, int, 32, 4, 8, 4x8, ROW_MAJOR, , 8, true, notTF32)
DEFINE_STORE(Accumulator_RowMajor, , int, 32, int, 32, 3, 8, 3x8, ROW_MAJOR, , 8, true, notTF32)
DEFINE_STORE(Accumulator_RowMajor, , int, 32, int, 32, 2, 8, 2x8, ROW_MAJOR, , 8, true, notTF32)
DEFINE_STORE(Accumulator_RowMajor, , int, 32, int, 32, 1, 8, 1x8, ROW_MAJOR, , 8, true, notTF32)

DEFINE_STORE(Accumulator_RowMajor,      , int, 32, int, 32, 8, 16, 8x16, ROW_MAJOR, , 16, true, notTF32)

DEFINE_STORE(Accumulator_RowMajor, _SG16, int, 32, int, 32, 8, 16, 8x16, ROW_MAJOR, , 16, true, notTF32)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, 32, int, 32, 7, 16, 7x16, ROW_MAJOR, , 16, true, notTF32)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, 32, int, 32, 6, 16, 6x16, ROW_MAJOR, , 16, true, notTF32)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, 32, int, 32, 5, 16, 5x16, ROW_MAJOR, , 16, true, notTF32)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, 32, int, 32, 4, 16, 4x16, ROW_MAJOR, , 16, true, notTF32)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, 32, int, 32, 3, 16, 3x16, ROW_MAJOR, , 16, true, notTF32)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, 32, int, 32, 2, 16, 2x16, ROW_MAJOR, , 16, true, notTF32)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, 32, int, 32, 1, 16, 1x16, ROW_MAJOR, , 16, true, notTF32)

DEFINE_STORE(Accumulator_ColumnMajor,      , int, 32, int, 32, 8, 8, 8x8, COL_MAJOR, , -1, false, notTF32)
DEFINE_STORE(Accumulator_ColumnMajor, _SG16, int, 32, int, 32, 8, 8, 8x8, COL_MAJOR, , -1, false, notTF32)


// get_coord()
#define MANGLE_GETCOORD_NAME(layout, sg, elem_bitwidth, shape) \
  __builtin_spirv_OpJointMatrixGetCoordINTEL_##layout##sg##_##shape##_i##elem_bitwidth

/* Explanation of calculation for int8 and bf16 types
Let's say we are considering a JM of use::A, 8x32, of type i8, in Platform PVC.

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

So the calculation becomes

int div_factor = 32 / 16 * 1; // --> 2
int row = index / 2; // 1
int col = (index % 2) + (wi_num * 2); // 1


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

Please note the index of the starred item is 3, not 9.
*/

#define DEFINE_GET_COORD(layout, sg, elem_bitwidth, M, K, shape, sg_size, VF) \
  INLINE int2 MANGLE_GETCOORD_NAME(layout, sg, elem_bitwidth, shape) (int index) { \
    int wi_num = get_sub_group_local_id(); \
    int div_factor = (K/sg_size)*VF; \
    int row = index/div_factor; \
    int col = (index%div_factor) + (wi_num*div_factor); \
    int2 result = (int2)(row, col); \
    return result; \
  }

// ------  PVC -------
// layout, sg, elem_bitwidth, M, K, shape, sg_size, VF
//int8
DEFINE_GET_COORD(PackedA, _SG16, 8, 8, 32, 8x32, 16, 1)
DEFINE_GET_COORD(PackedB, _SG16, 8, 32, 16, 32x16, 16, 4)
DEFINE_GET_COORD(Accumulator, _SG16, 32, 8, 16, 8x16, 16, 1)

//bfloat16
DEFINE_GET_COORD(PackedA, _SG16, 16, 8, 16, 8x16, 16, 1)
DEFINE_GET_COORD(PackedB, _SG16, 16, 16, 16, 32x16, 16, 2)



// --------- XMX8 ------------
//int8
DEFINE_GET_COORD(PackedA, , 8, 8, 32, 8x32, 8, 1)
DEFINE_GET_COORD(PackedB, , 8, 32, 8, 32x8, 8, 4)
DEFINE_GET_COORD(Accumulator, , 32, 8, 8, 8x8, 8, 1)

//bfloat16
DEFINE_GET_COORD(PackedA, , 16, 8, 16, 8x16, 8, 1)
DEFINE_GET_COORD(PackedB, , 16, 16, 8, 16x8, 8, 2)

/* experimental large slice support: */

INLINE void __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_bf16_bf16_fp32(__private char *a_ptr, __private char *b_ptr, __private char *raw_c_ptr, __private char *result) {
    short16 a   = *(short16 *)a_ptr;
    int8 b      = *(int8 *)b_ptr;
    int16 raw_c = *(int16 *)raw_c_ptr;

    short8 a0 = (short8)(a.s0, a.s1, a.s2, a.s3, a.s4, a.s5, a.s6, a.s7);
    short8 a1 = (short8)(a.s8, a.s9, a.sa, a.sb, a.sc, a.sd, a.se, a.sf);

    float16 c = *(float16 *)&raw_c;

    float8 c0 = (float8)(c.s0, c.s1, c.s2, c.s3, c.s4, c.s5, c.s6, c.s7);
    float8 c1 = (float8)(c.s8, c.s9, c.sa, c.sb, c.sc, c.sd, c.se, c.sf);

    float8 fres0 = __builtin_IB_sub_group16_fdpas_f_f_bf_bf_8_8(c0, a0, b);
    float8 fres1 = __builtin_IB_sub_group16_fdpas_f_f_bf_bf_8_8(c0, a1, b);

    int8 res0 = *(int8 *)&fres0;
    int8 res1 = *(int8 *)&fres1;

    __private int16 *dst = (__private int16 *)result;
    *dst = (int16)(res0.s0, res0.s1, res0.s2, res0.s3, res0.s4, res0.s5, res0.s6, res0.s7,
                   res1.s0, res1.s1, res1.s2, res1.s3, res1.s4, res1.s5, res1.s6, res1.s7);
}

INLINE void __builtin_spriv_OpJointMatrixMadINTEL_16x16x16_fp16_fp16_fp32(__private char *a_ptr, __private char *b_ptr, __private char *raw_c_ptr, __private char *result) {
    short16 a   = *(short16 *)a_ptr;
    int8 b      = *(int8 *)b_ptr;
    int16 raw_c = *(int16 *)raw_c_ptr;

    short8 a0 = (short8)(a.s0, a.s1, a.s2, a.s3, a.s4, a.s5, a.s6, a.s7);
    short8 a1 = (short8)(a.s8, a.s9, a.sa, a.sb, a.sc, a.sd, a.se, a.sf);

    float16 c = *(float16 *)&raw_c;

    float8 c0 = (float8)(c.s0, c.s1, c.s2, c.s3, c.s4, c.s5, c.s6, c.s7);
    float8 c1 = (float8)(c.s8, c.s9, c.sa, c.sb, c.sc, c.sd, c.se, c.sf);

    float8 fres0 = __builtin_IB_sub_group16_fdpas_f_f_hf_hf_8_8(c0, a0, b);
    float8 fres1 = __builtin_IB_sub_group16_fdpas_f_f_hf_hf_8_8(c0, a1, b);

    int8 res0 = *(int8 *)&fres0;
    int8 res1 = *(int8 *)&fres1;

    __private int16 *dst = (__private int16 *)result;
    *dst = (int16)(res0.s0, res0.s1, res0.s2, res0.s3, res0.s4, res0.s5, res0.s6, res0.s7,
                   res1.s0, res1.s1, res1.s2, res1.s3, res1.s4, res1.s5, res1.s6, res1.s7);
}

INLINE void __builtin_spriv_OpJointMatrixMadINTEL_32x64x16_bf16_bf16_fp32(__private char *a_ptr, __private char *b_ptr, __private char *c_ptr, __private char *d_ptr) {
    __private char *a0 = a_ptr;
    __private char *a1 = a_ptr + 16 * (sizeof (short));

    __private char *b0 = b_ptr;
    __private char *b1 = b_ptr + 1 * 16 * (sizeof (short));
    __private char *b2 = b_ptr + 2 * 16 * (sizeof (short));
    __private char *b3 = b_ptr + 3 * 16 * (sizeof (short));

    __private char *c0 = c_ptr + 0 * 16 * (sizeof (int));
    __private char *c1 = c_ptr + 1 * 16 * (sizeof (int));
    __private char *c2 = c_ptr + 2 * 16 * (sizeof (int));
    __private char *c3 = c_ptr + 3 * 16 * (sizeof (int));
    __private char *c4 = c_ptr + 4 * 16 * (sizeof (int));
    __private char *c5 = c_ptr + 5 * 16 * (sizeof (int));
    __private char *c6 = c_ptr + 6 * 16 * (sizeof (int));
    __private char *c7 = c_ptr + 7 * 16 * (sizeof (int));

    __private char *d0 = d_ptr + 0 * 16 * (sizeof (int));
    __private char *d1 = d_ptr + 1 * 16 * (sizeof (int));
    __private char *d2 = d_ptr + 2 * 16 * (sizeof (int));
    __private char *d3 = d_ptr + 3 * 16 * (sizeof (int));
    __private char *d4 = d_ptr + 4 * 16 * (sizeof (int));
    __private char *d5 = d_ptr + 5 * 16 * (sizeof (int));
    __private char *d6 = d_ptr + 6 * 16 * (sizeof (int));
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

INLINE void __builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_v8i8_pi32_i32(__private char *dst, char *mem, long stride) {
    IMPLEMENT_BLOCK2D_LOAD_SG16_ROW_MAJOR(int, 32, int, 32, 16, 16, 16)
}

INLINE void __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_16x16_i16_generic_v8i8_pi32_i32(__private char *dst, char *mem, long stride) {
    IMPLEMENT_BLOCK2D_LOAD_SG16_ROW_MAJOR(short, 16, short, 16, 16, 16, 16)
}

INLINE void __builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_32x64_i32_generic_v8i8_pi32_i32(__private char *dst, char *mem, long stride) {
    __private char *c0 = dst + 0 * 16 * (sizeof (int));
    __private char *c1 = dst + 1 * 16 * (sizeof (int));
    __private char *c2 = dst + 2 * 16 * (sizeof (int));
    __private char *c3 = dst + 3 * 16 * (sizeof (int));
    __private char *c4 = dst + 4 * 16 * (sizeof (int));
    __private char *c5 = dst + 5 * 16 * (sizeof (int));
    __private char *c6 = dst + 6 * 16 * (sizeof (int));
    __private char *c7 = dst + 7 * 16 * (sizeof (int));

    char *mem0 = mem + 0 * 16 * (sizeof (int));
    char *mem1 = mem + 1 * 16 * (sizeof (int));
    char *mem2 = mem + 2 * 16 * (sizeof (int));
    char *mem3 = mem + 3 * 16 * (sizeof (int));
    char *mem4 = mem + 0 * 16 * (sizeof (int)) + 16 * (sizeof (int)) * stride;
    char *mem5 = mem + 1 * 16 * (sizeof (int)) + 16 * (sizeof (int)) * stride;
    char *mem6 = mem + 2 * 16 * (sizeof (int)) + 16 * (sizeof (int)) * stride;
    char *mem7 = mem + 3 * 16 * (sizeof (int)) + 16 * (sizeof (int)) * stride;

    __builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_v8i8_pi32_i32(c0, mem0, stride);
    __builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_v8i8_pi32_i32(c1, mem1, stride);
    __builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_v8i8_pi32_i32(c2, mem2, stride);
    __builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_v8i8_pi32_i32(c3, mem3, stride);
    __builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_v8i8_pi32_i32(c4, mem4, stride);
    __builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_v8i8_pi32_i32(c5, mem5, stride);
    __builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_v8i8_pi32_i32(c6, mem6, stride);
    __builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_v8i8_pi32_i32(c7, mem7, stride);
}

INLINE void __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_32x16_i16_generic_v8i8_pi32_i32(__private char *dst, char *mem, long stride) {
    __private char *dst0 = dst;
    __private char *dst1 = dst + 16 * (sizeof (short));

    char *mem0 = mem;
    char *mem1 = mem + 16 * (sizeof (short)) * stride;

    __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_16x16_i16_generic_v8i8_pi32_i32(dst0, mem0, stride);
    __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_16x16_i16_generic_v8i8_pi32_i32(dst1, mem1, stride);
}

INLINE void __builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_16x64_i16_generic_v8i8_pi32_i32(__private char *dst, char *mem, long stride) {
    __private char *b0 = dst;
    __private char *b1 = dst + 1 * 16 * (sizeof (short));
    __private char *b2 = dst + 2 * 16 * (sizeof (short));
    __private char *b3 = dst + 3 * 16 * (sizeof (short));

    char *mem0 = mem + 0 * 16 * (sizeof (int));
    char *mem1 = mem + 1 * 16 * (sizeof (int));
    char *mem2 = mem + 2 * 16 * (sizeof (int));
    char *mem3 = mem + 3 * 16 * (sizeof (int));

    __builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_16x16_i16_generic_v8i8_pi32_i32(b0, mem0, stride);
    __builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_16x16_i16_generic_v8i8_pi32_i32(b1, mem1, stride);
    __builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_16x16_i16_generic_v8i8_pi32_i32(b2, mem2, stride);
    __builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_16x16_i16_generic_v8i8_pi32_i32(b3, mem3, stride);
}

INLINE void __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_pi64_v8i8(char *mem, __private char *src, long stride) {
    IMPLEMENT_BLOCK2D_STORE_SG16(int, int, 32, 16, 16, slice)
}

INLINE void __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_generic_pi64_v8i8(char *mem, __private char *src, long stride) {
    __private char *c0 = src + 0 * 16 * (sizeof (int));
    __private char *c1 = src + 1 * 16 * (sizeof (int));
    __private char *c2 = src + 2 * 16 * (sizeof (int));
    __private char *c3 = src + 3 * 16 * (sizeof (int));
    __private char *c4 = src + 4 * 16 * (sizeof (int));
    __private char *c5 = src + 5 * 16 * (sizeof (int));
    __private char *c6 = src + 6 * 16 * (sizeof (int));
    __private char *c7 = src + 7 * 16 * (sizeof (int));

    char *mem0 = mem + 0 * 16 * (sizeof (int));
    char *mem1 = mem + 1 * 16 * (sizeof (int));
    char *mem2 = mem + 2 * 16 * (sizeof (int));
    char *mem3 = mem + 3 * 16 * (sizeof (int));
    char *mem4 = mem + 0 * 16 * (sizeof (int)) + 16 * (sizeof (int)) * stride;
    char *mem5 = mem + 1 * 16 * (sizeof (int)) + 16 * (sizeof (int)) * stride;
    char *mem6 = mem + 2 * 16 * (sizeof (int)) + 16 * (sizeof (int)) * stride;
    char *mem7 = mem + 3 * 16 * (sizeof (int)) + 16 * (sizeof (int)) * stride;

    __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_pi64_v8i8(mem0, c0, stride);
    __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_pi64_v8i8(mem1, c1, stride);
    __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_pi64_v8i8(mem2, c2, stride);
    __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_pi64_v8i8(mem3, c3, stride);
    __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_pi64_v8i8(mem4, c4, stride);
    __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_pi64_v8i8(mem5, c5, stride);
    __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_pi64_v8i8(mem6, c6, stride);
    __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_generic_pi64_v8i8(mem7, c7, stride);
}

