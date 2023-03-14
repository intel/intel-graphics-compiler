/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Optimized implementation of Joint Matrix Load/Store built-ins
#define SCALAR_IMPL 0 // always scalar
#define VECTOR_IMPL 1  // always vector for row major
#define VECTOR_CONT_IMPL 2  // vector optimized for contiguos memory access, where possible (default)
extern __constant int __JointMatrixLoadStoreOpt;

// Matrix order
#define ROW_MAJOR 0
#define COL_MAJOR 1

#define IND_ROW_MAJOR(slid, stride, i) (slid + (i * stride))
#define IND_COL_MAJOR(slid, stride, i) (i + (slid * stride))

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

#define OUT_VEC8(type) type##8
#define OUT_VEC7(type) type##8
#define OUT_VEC6(type) type##8
#define OUT_VEC5(type) type##8
#define OUT_VEC4(type) type##4
#define OUT_VEC3(type) type##3
#define OUT_VEC2(type) type##2
#define OUT_VEC1(type) type

// layout can be PackedA_RowMajor, PackedB_ColumnMajor, PackedB_PackedB, etc.
// sg is empty for XMX8 and _SG16 for PVC
// elem_name is i8, i16 or i32
// shape is shape of the matrix, like 8x16
#define MANGLE_LOAD_NAME(layout, sg, elem_name, shape) \
  __builtin_spriv_OpJointMatrixLoadINTEL_##layout##sg##_##shape##_##elem_name##_v8i8_pi32_i32

#define SUB_GROUP_LOADS_8(readop, ptr, stride, type) \
    (type##8)(readop((ptr) + 0 * (stride)), readop((ptr) + 1 * (stride)), \
              readop((ptr) + 2 * (stride)), readop((ptr) + 3 * (stride)), \
              readop((ptr) + 4 * (stride)), readop((ptr) + 5 * (stride)), \
              readop((ptr) + 6 * (stride)), readop((ptr) + 7 * (stride)));

#define SUB_GROUP_LOADS_7(readop, ptr, stride, type) \
    (type##8)(readop((ptr) + 0 * (stride)), readop((ptr) + 1 * (stride)), \
              readop((ptr) + 2 * (stride)), readop((ptr) + 3 * (stride)), \
              readop((ptr) + 4 * (stride)), readop((ptr) + 5 * (stride)), \
              readop((ptr) + 6 * (stride)), 0);

#define SUB_GROUP_LOADS_6(readop, ptr, stride, type) \
    (type##8)(readop((ptr) + 0 * (stride)), readop((ptr) + 1 * (stride)), \
              readop((ptr) + 2 * (stride)), readop((ptr) + 3 * (stride)), \
              readop((ptr) + 4 * (stride)), readop((ptr) + 5 * (stride)), \
              0, 0);

#define SUB_GROUP_LOADS_5(readop, ptr, stride, type) \
    (type##8)(readop((ptr) + 0 * (stride)), readop((ptr) + 1 * (stride)), \
              readop((ptr) + 2 * (stride)), readop((ptr) + 3 * (stride)), \
              readop((ptr) + 4 * (stride)), 0, 0, 0);

#define SUB_GROUP_LOADS_4(readop, ptr, stride, type) \
    (type##4)(readop((ptr) + 0 * (stride)), readop((ptr) + 1 * (stride)), \
              readop((ptr) + 2 * (stride)), readop((ptr) + 3 * (stride)));

#define SUB_GROUP_LOADS_3(readop, ptr, stride, type) \
    (type##3)(readop((ptr) + 0 * (stride)), readop((ptr) + 1 * (stride)), \
              readop((ptr) + 2 * (stride)));

#define SUB_GROUP_LOADS_2(readop, ptr, stride, type) \
    (type##2)(readop((ptr) + 0 * (stride)), readop((ptr) + 1 * (stride)));

#define SUB_GROUP_LOADS_1(readop, ptr, stride, type) (type)(readop(ptr));

// variants for 7,6,5,3 and 1 are only used to make the code compilable
#define DEFINE_BLOCK_RW_NAME8(rw, us) intel_sub_group_block_##rw##us##8
#define DEFINE_BLOCK_RW_NAME7(rw, us) intel_sub_group_block_##rw##us##8
#define DEFINE_BLOCK_RW_NAME6(rw, us) intel_sub_group_block_##rw##us##8
#define DEFINE_BLOCK_RW_NAME5(rw, us) intel_sub_group_block_##rw##us##8
#define DEFINE_BLOCK_RW_NAME4(rw, us) intel_sub_group_block_##rw##us##4
#define DEFINE_BLOCK_RW_NAME3(rw, us) intel_sub_group_block_##rw##us##4
#define DEFINE_BLOCK_RW_NAME2(rw, us) intel_sub_group_block_##rw##us##2
#define DEFINE_BLOCK_RW_NAME1(rw, us) intel_sub_group_block_##rw##us

// layout can be PackedA_RowMajor, PackedB_ColumnMajor, PackedB_PackedB, etc.
// sg is empty for XMX8 and _SG16 for PVC
// element_type is char for i8, short for i16 and int for i32
// elem_name is i8, i16 or i32
// contrib_type is int or short depending on available OpenCL extension API
// M is number of rows
// shape is shape of the matrix, like 8x16. We can not replace shape with M and stride_opt parameters,
//      in case of vnni'd B, so we keep it.
// order is ROW_MAJOR or COL_MAJOR
// us is empty for int contrib type and _us for short contrib type.
// stride_opt should be either equal to C or 2*C in case of matrix B, since matrix B is vnni'ed
#define DEFINE_LOAD(layout, sg, element_type, elem_name, contrib_type, M, shape, order, us, stride_opt) \
  INLINE OUT_VEC##M(contrib_type) MANGLE_LOAD_NAME(layout, sg, elem_name, shape) (char *mem, int stride) { \
      if (__JointMatrixLoadStoreOpt == VECTOR_CONT_IMPL && stride == stride_opt \
          && (M == 2 || M == 4 || M == 8) && order == ROW_MAJOR) { \
          return VEC_TO_VEC##M(contrib_type, DEFINE_BLOCK_RW_NAME##M(read, us)((__global u##contrib_type *) mem)); \
      } \
      if ((__JointMatrixLoadStoreOpt == VECTOR_IMPL || __JointMatrixLoadStoreOpt == VECTOR_CONT_IMPL) \
          && order == ROW_MAJOR) { \
          int pack_factor = sizeof (u##contrib_type) / sizeof (element_type); \
          stride = stride / pack_factor; \
          return SUB_GROUP_LOADS_##M(intel_sub_group_block_read##us, (__global u##contrib_type *)mem, stride, contrib_type) \
      } \
      contrib_type *ptr = (contrib_type *)mem; \
      int slid = get_sub_group_local_id(); \
      int pack_factor = sizeof (contrib_type) / sizeof (element_type); \
      stride = stride / pack_factor; \
      contrib_type wi_contrib[M]; \
      for (int i = 0; i < M; i++) \
          wi_contrib[i] = *( ptr + IND_##order(slid, stride, i) ); \
      return ARR_TO_VEC##M(contrib_type, wi_contrib); \
  }

/* PackedA load i16 */
DEFINE_LOAD(PackedA_RowMajor, , short, i16, int, 8, 8x16, ROW_MAJOR, , 16)
DEFINE_LOAD(PackedA_RowMajor, , short, i16, int, 7, 7x16, ROW_MAJOR, , 16)
DEFINE_LOAD(PackedA_RowMajor, , short, i16, int, 6, 6x16, ROW_MAJOR, , 16)
DEFINE_LOAD(PackedA_RowMajor, , short, i16, int, 5, 5x16, ROW_MAJOR, , 16)
DEFINE_LOAD(PackedA_RowMajor, , short, i16, int, 4, 4x16, ROW_MAJOR, , 16)
DEFINE_LOAD(PackedA_RowMajor, , short, i16, int, 3, 3x16, ROW_MAJOR, , 16)
DEFINE_LOAD(PackedA_RowMajor, , short, i16, int, 2, 2x16, ROW_MAJOR, , 16)
DEFINE_LOAD(PackedA_RowMajor, , short, i16, int, 1, 1x16, ROW_MAJOR, , 16)

/* PackedA load i8 */
DEFINE_LOAD(PackedA_RowMajor, , char, i8, int, 8, 8x32, ROW_MAJOR, , 32)
DEFINE_LOAD(PackedA_RowMajor, , char, i8, int, 7, 7x32, ROW_MAJOR, , 32)
DEFINE_LOAD(PackedA_RowMajor, , char, i8, int, 6, 6x32, ROW_MAJOR, , 32)
DEFINE_LOAD(PackedA_RowMajor, , char, i8, int, 5, 5x32, ROW_MAJOR, , 32)
DEFINE_LOAD(PackedA_RowMajor, , char, i8, int, 4, 4x32, ROW_MAJOR, , 32)
DEFINE_LOAD(PackedA_RowMajor, , char, i8, int, 3, 3x32, ROW_MAJOR, , 32)
DEFINE_LOAD(PackedA_RowMajor, , char, i8, int, 2, 2x32, ROW_MAJOR, , 32)
DEFINE_LOAD(PackedA_RowMajor, , char, i8, int, 1, 1x32, ROW_MAJOR, , 32)

/* PackedA load i16 SG16 */
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, i16, short, 8, 8x16, ROW_MAJOR, _us, 16)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, i16, short, 7, 7x16, ROW_MAJOR, _us, 16)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, i16, short, 6, 6x16, ROW_MAJOR, _us, 16)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, i16, short, 5, 5x16, ROW_MAJOR, _us, 16)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, i16, short, 4, 4x16, ROW_MAJOR, _us, 16)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, i16, short, 3, 3x16, ROW_MAJOR, _us, 16)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, i16, short, 2, 2x16, ROW_MAJOR, _us, 16)
DEFINE_LOAD(PackedA_RowMajor, _SG16, short, i16, short, 1, 1x16, ROW_MAJOR, _us, 16)

/* PackedA load i8 SG16 */
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, i8, short, 8, 8x32, ROW_MAJOR, _us, 32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, i8, short, 7, 7x32, ROW_MAJOR, _us, 32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, i8, short, 6, 6x32, ROW_MAJOR, _us, 32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, i8, short, 5, 5x32, ROW_MAJOR, _us, 32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, i8, short, 4, 4x32, ROW_MAJOR, _us, 32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, i8, short, 3, 3x32, ROW_MAJOR, _us, 32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, i8, short, 2, 2x32, ROW_MAJOR, _us, 32)
DEFINE_LOAD(PackedA_RowMajor, _SG16, char, i8, short, 1, 1x32, ROW_MAJOR, _us, 32)

/* PackedB load i16 */
DEFINE_LOAD(PackedB_ColumnMajor, , short, i16, int, 8, 16x8, COL_MAJOR, , -1)
DEFINE_LOAD(PackedB_PackedB, , short, i16, int, 8, 16x8, ROW_MAJOR, , 16)
DEFINE_LOAD(PackedB_PackedB, , short, i16, int, 8, 16x16, ROW_MAJOR, , 32)

/* PackedB load i8 */
DEFINE_LOAD(PackedB_ColumnMajor, , char, i8, int, 8, 32x8, COL_MAJOR, , -1)
DEFINE_LOAD(PackedB_PackedB, , char, i8, int, 8, 32x8, ROW_MAJOR, , 16)
DEFINE_LOAD(PackedB_PackedB, , char, i8, int, 8, 32x16, ROW_MAJOR, , 32)

/* Load accumulator is a special case of load packed A, both are row major: */
DEFINE_LOAD(Accumulator_RowMajor, , int, i32, int, 8, 8x8, ROW_MAJOR, , 8)
DEFINE_LOAD(Accumulator_RowMajor, , int, i32, int, 7, 7x8, ROW_MAJOR, , 8)
DEFINE_LOAD(Accumulator_RowMajor, , int, i32, int, 6, 6x8, ROW_MAJOR, , 8)
DEFINE_LOAD(Accumulator_RowMajor, , int, i32, int, 5, 5x8, ROW_MAJOR, , 8)
DEFINE_LOAD(Accumulator_RowMajor, , int, i32, int, 4, 4x8, ROW_MAJOR, , 8)
DEFINE_LOAD(Accumulator_RowMajor, , int, i32, int, 3, 3x8, ROW_MAJOR, , 8)
DEFINE_LOAD(Accumulator_RowMajor, , int, i32, int, 2, 2x8, ROW_MAJOR, , 8)
DEFINE_LOAD(Accumulator_RowMajor, , int, i32, int, 1, 1x8, ROW_MAJOR, , 8)

DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, i32, int, 8, 8x16, ROW_MAJOR, , 16)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, i32, int, 7, 7x16, ROW_MAJOR, , 16)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, i32, int, 6, 6x16, ROW_MAJOR, , 16)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, i32, int, 5, 5x16, ROW_MAJOR, , 16)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, i32, int, 4, 4x16, ROW_MAJOR, , 16)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, i32, int, 3, 3x16, ROW_MAJOR, , 16)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, i32, int, 2, 2x16, ROW_MAJOR, , 16)
DEFINE_LOAD(Accumulator_RowMajor, _SG16, int, i32, int, 1, 1x16, ROW_MAJOR, , 16)

// --------- STORE built-ins --------------------------------------

#define MANGLE_STORE_NAME(layout, sg, elem_name, shape) \
  __builtin_spriv_OpJointMatrixStoreINTEL_##layout##sg##_##shape##_##elem_name##_pi64_v8i8

#define VEC_IND8(var, ind) var[ind]
#define VEC_IND7(var, ind) var[ind]
#define VEC_IND6(var, ind) var[ind]
#define VEC_IND5(var, ind) var[ind]
#define VEC_IND4(var, ind) var[ind]
#define VEC_IND3(var, ind) var[ind]
#define VEC_IND2(var, ind) var[ind]
#define VEC_IND1(var, ind) var

// set block_opt to false to disable block non-continous optimization per one built-in as a workaround
#define DEFINE_STORE(layout, sg, element_type, elem_name, contrib_type, M, shape, order, us, stride_opt, block_opt) \
  INLINE void MANGLE_STORE_NAME(layout, sg, elem_name, shape) (char *mem, OUT_VEC##M(contrib_type) vec, int stride) { \
      if (__JointMatrixLoadStoreOpt == VECTOR_CONT_IMPL && stride == stride_opt \
          && (M == 2 || M == 4 || M == 8) && order == ROW_MAJOR) { \
          DEFINE_BLOCK_RW_NAME##M(write, us)((__global u##contrib_type *)mem, VEC_TO_VEC_STORE##M( u##contrib_type , vec)); \
          return; \
      } \
      if ((__JointMatrixLoadStoreOpt == VECTOR_IMPL || __JointMatrixLoadStoreOpt == VECTOR_CONT_IMPL) \
          && order == ROW_MAJOR && block_opt == true) { \
          __global u##contrib_type *ptr = (__global u##contrib_type *)mem; \
          int pack_factor = sizeof (u##contrib_type) / sizeof (element_type); \
          stride = stride / pack_factor; \
          for (int i = 0; i < M; i++) \
              intel_sub_group_block_write##us(ptr + i * stride, (u##contrib_type) VEC_IND##M(vec, i)); \
          return; \
      } \
      contrib_type *ptr = (contrib_type *)mem; \
      int slid = get_sub_group_local_id(); \
      int pack_factor = sizeof (contrib_type) / sizeof (element_type); \
      stride = stride / pack_factor; \
      for (int i = 0; i < M; i++) \
          ptr[ IND_##order(slid, stride, i) ] = VEC_IND##M(vec, i); \
  }

// TODO: investigate why intel_sub_group_block_write causes an assertion and enable blocked non-continuous optimization
DEFINE_STORE(PackedA_RowMajor, , char, i8, int, 8, 8x32, ROW_MAJOR, , 32, false)

// TODO: investigate why intel_sub_group_block_write causes an assertion and enable blocked non-continuous optimization
DEFINE_STORE(PackedA_RowMajor, , short, i16, int, 8, 8x16, ROW_MAJOR, , 16, false)

// TODO: investigate why intel_sub_group_block_write_us causes an assertion and enable blocked non-continuous optimization
DEFINE_STORE(PackedA_RowMajor, _SG16, char, i8, short, 8, 8x32, ROW_MAJOR, _us, 32, false)

// TODO: investigate why intel_sub_group_block_write_us causes an assertion and enable blocked non-continuous optimization
DEFINE_STORE(PackedA_RowMajor, _SG16, short, i16, short, 8, 8x16, ROW_MAJOR, _us, 16, false)

DEFINE_STORE(PackedB_PackedB, , short, i16, int, 8, 16x8, ROW_MAJOR, , 16, true)
DEFINE_STORE(PackedB_PackedB, , short, i16, int, 8, 16x16, ROW_MAJOR, , 32, true)

// TODO: investigate why intel_sub_group_block_write causes an assertion and enable blocked non-continuous optimization
DEFINE_STORE(PackedB_PackedB, , char, i8, int, 8, 32x8, ROW_MAJOR, , 16, false)

DEFINE_STORE(Accumulator_RowMajor, , int, i32, int, 8, 8x8, ROW_MAJOR, , 8, true)
DEFINE_STORE(Accumulator_RowMajor, , int, i32, int, 7, 7x8, ROW_MAJOR, , 8, true)
DEFINE_STORE(Accumulator_RowMajor, , int, i32, int, 6, 6x8, ROW_MAJOR, , 8, true)
DEFINE_STORE(Accumulator_RowMajor, , int, i32, int, 5, 5x8, ROW_MAJOR, , 8, true)
DEFINE_STORE(Accumulator_RowMajor, , int, i32, int, 4, 4x8, ROW_MAJOR, , 8, true)
DEFINE_STORE(Accumulator_RowMajor, , int, i32, int, 3, 3x8, ROW_MAJOR, , 8, true)
DEFINE_STORE(Accumulator_RowMajor, , int, i32, int, 2, 2x8, ROW_MAJOR, , 8, true)
DEFINE_STORE(Accumulator_RowMajor, , int, i32, int, 1, 1x8, ROW_MAJOR, , 8, true)

DEFINE_STORE(Accumulator_RowMajor, , int, i32, int, 8, 8x16, ROW_MAJOR, , 16, true)

DEFINE_STORE(Accumulator_RowMajor, _SG16, int, i32, int, 8, 8x16, ROW_MAJOR, , 16, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, i32, int, 7, 7x16, ROW_MAJOR, , 16, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, i32, int, 6, 6x16, ROW_MAJOR, , 16, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, i32, int, 5, 5x16, ROW_MAJOR, , 16, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, i32, int, 4, 4x16, ROW_MAJOR, , 16, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, i32, int, 3, 3x16, ROW_MAJOR, , 16, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, i32, int, 2, 2x16, ROW_MAJOR, , 16, true)
DEFINE_STORE(Accumulator_RowMajor, _SG16, int, i32, int, 1, 1x16, ROW_MAJOR, , 16, true)

DEFINE_STORE(Accumulator_ColumnMajor, , int, i32, int, 8, 8x8, COL_MAJOR, , -1, false)
